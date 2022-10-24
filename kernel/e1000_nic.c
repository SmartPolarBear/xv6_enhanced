//
// Created by bear on 10/19/2022.
//

#include "types.h"
#include "debug.h"
#include "defs.h"
#include "mmu.h"
#include "slab.h"
#include "list.h"

#include "pci.h"
#include "e1000_nic.h"
#include "net.h"

int e1000_net_init(void *state, void *hwaddr);
int e1000_net_send(void *state, const void *data, int len);
int e1000_net_recv(void *state, void *data, int len);

netcard_opts_t e1000_opts = {
	.init = e1000_net_init,
	.send = e1000_net_send,
	.receive = e1000_net_recv,
};

typedef struct e1000
{
	struct TD *transmit_desc_list[32];
	struct RD *receive_desc_list[128];

	struct packet *tbuf[32];
	struct packet *rbuf[128];

	volatile uint32 *mmio_base;
	size_t mmio_size;
} e1000_t;

kmem_cache_t *e1000_cache = NULL;
size_t e1000_count = 0;

static void e1000_reg_write(uint32 reg_addr, uint32 value, e1000_t *the_e1000)
{
	*(uint32 *)(the_e1000->mmio_base + reg_addr) = value;
}

static uint32 e1000_reg_read(uint32 reg_addr, e1000_t *the_e1000)
{
	uint32 value = *(uint32 *)(the_e1000->mmio_base + reg_addr);
	return value;
}

// x86 trick: each inb of port 0x84 takes about 1.25us
// Super stupid delay logic. Don't even know if this works
// or understand what port 0x84 does.
// Could not find an explanantion.
static void udelay(unsigned int u)
{
	unsigned int i;
	for (i = 0; i < u; i++)
		inb(0x84);
}

void e1000_init(void)
{
	e1000_cache = kmem_cache_create("e1000", sizeof(e1000_t), 0);
	if (!e1000_cache)
	{
		panic("e1000_init: kmem_cache_create failed");
	}
}

int e1000_nic_attach(struct pci_func *pcif)
{
	// Enable PCI function
	pci_func_enable(pcif);

	// Map MMIO region
	uint32 mmio_base = pcif->reg_base[0];
	uint32 mmio_size = pcif->reg_size[0];

	cprintf("e1000: base: 0x%x size: 0x%x status reg: %x\n", mmio_base, mmio_size, *(((uint32 *)mmio_base) + 2));

	// Allocate card
	e1000_t *card = (e1000_t *)kmem_cache_alloc(e1000_cache);
	memset(card, 0, sizeof(e1000_t));

	card->mmio_base = (uint32 *)mmio_base;
	card->mmio_size = mmio_size;

	uint32 id = e1000_count++;
	char name[8] = "e1000_";
	name[6] = '0' + id;

	nic_register(name, pcif, &e1000_opts, card);

	// Reset device but keep the PCI config
	e1000_reg_write(E1000_CNTRL_REG,
					e1000_reg_read(E1000_CNTRL_REG, the_e1000) | E1000_CNTRL_RST_MASK,
					card);
	//read back the value after approx 1us to check RST bit cleared
	do
	{
		udelay(3);
	} while (E1000_CNTRL_RST_BIT(e1000_reg_read(E1000_CNTRL_REG, the_e1000)));

	//the manual says in Section 14.3 General Config -
	//Must set the ASDE and SLU(bit 5 and 6(0 based index)) in the CNTRL Reg to allow auto speed
	//detection after RESET
	uint32 cntrl_reg = e1000_reg_read(E1000_CNTRL_REG, the_e1000);
	e1000_reg_write(E1000_CNTRL_REG, cntrl_reg | E1000_CNTRL_ASDE_MASK | E1000_CNTRL_SLU_MASK,
					card);

	//Read Hardware(MAC) address from the device
	uint32 macaddr_l = e1000_reg_read(E1000_RCV_RAL0, the_e1000);
	uint32 macaddr_h = e1000_reg_read(E1000_RCV_RAH0, the_e1000);
	*(uint32 *)card->mac_addr = macaddr_l;
	*(uint16 * )(&the_e1000->mac_addr[4]) = (uint16)macaddr_h;
	*(uint32 *)mac_addr = macaddr_l;
	*(uint32 * )(&mac_addr[4]) = (uint16)macaddr_h;

	char mac_str[18];
	unpack_mac(card->mac_addr, mac_str);
	mac_str[17] = 0;
	cprintf("\nMAC address of the e1000 device:%s\n", mac_str);
	//Transmit/Receive and DMA config beyond this point...
	//sizeof(tbd)=128bits/16bytes. so 256 of these will fit in a page of size 4KB.
	//But the struct e1000 has to contain pointer to these many descriptors.
	//Each pointer being 4 bytes, and 4 such array of pointers(inclusing packet buffers)
	//you get N*16+(some more values in the struct e1000) = 4096
	// N=128=E1000_TBD_SLOTS. i.e., the maximum number of descriptors in one ring
	struct e1000_tbd *ttmp = (struct e1000_tbd *)kalloc();
	for (int i = 0; i < E1000_TBD_SLOTS; i++, ttmp++)
	{
		card->tbd[i] = (struct e1000_tbd *)ttmp;
	}
	//check the last nibble of the transmit/receive rings to make sure they
	//are on paragraph boundary
	if ((V2P(the_e1000->tbd[0]) & 0x0000000f) != 0)
	{
		cprintf("ERROR:e1000:Transmit Descriptor Ring not on paragraph boundary\n");
		kfree((char *)ttmp);
		return -1;
	}
	//same for rbd
	struct e1000_rbd *rtmp = (struct e1000_rbd *)kalloc();
	for (int i = 0; i < E1000_RBD_SLOTS; i++, rtmp++)
	{
		card->rbd[i] = (struct e1000_rbd *)rtmp;
	}
	if ((V2P(card->rbd[0]) & 0x0000000f) != 0)
	{
		cprintf("ERROR:e1000:Receive Descriptor Ring not on paragraph boundary\n");
		kfree((char *)rtmp);
		return -1;
	}

	//Now for the packet buffers in Receive Ring. Can fit 2 packet buf in 1 page
	struct packet_buf *tmp;
	for (int i = 0; i < E1000_RBD_SLOTS; i += 2)
	{
		tmp = (struct packet_buf *)kalloc();
		card->rx_buf[i] = tmp++;
		card->rbd[i]->addr_l = V2P((uint32_t)the_e1000->rx_buf[i]);
		card->rbd[i]->addr_h = 0;
		card->rx_buf[i + 1] = tmp;
		card->rbd[i + 1]->addr_l = V2P((uint32_t)the_e1000->rx_buf[i + 1]);
		card->rbd[i + 1]->addr_h = 0;
	}
	for (int i = 0; i < E1000_TBD_SLOTS; i += 2)
	{
		tmp = (struct packet_buf *)kalloc();
		card->tx_buf[i] = tmp++;
		//the_e1000->tbd[i]->addr = (uint32_t)the_e1000->tx_buf[i];
		//the_e1000->tbd[i]->addr_h = 0;
		card->tx_buf[i + 1] = tmp;
		//the_e1000->tbd[i+1]->addr_l = (uint32_t)the_e1000->tx_buf[i+1];
		//the_e1000->tbd[i+1]->addr_h = 0;
	}

	//Write the Descriptor ring addresses in TDBAL, and RDBAL, plus HEAD and TAIL pointers
	e1000_reg_write(E1000_TDBAL, V2P(card->tbd[0]), card);
	e1000_reg_write(E1000_TDBAH, 0x00000000, card);
	e1000_reg_write(E1000_TDLEN, (E1000_TBD_SLOTS * 16) << 7, card);
	e1000_reg_write(E1000_TDH, 0x00000000, card);
	e1000_reg_write(E1000_TCTL,
					E1000_TCTL_EN |
						E1000_TCTL_PSP |
						E1000_TCTL_CT_SET(0x0f) |
						E1000_TCTL_COLD_SET(0x200),
					card);
	e1000_reg_write(E1000_TDT, 0, card);
	e1000_reg_write(E1000_TIPG,
					E1000_TIPG_IPGT_SET(10) |
						E1000_TIPG_IPGR1_SET(10) |
						E1000_TIPG_IPGR2_SET(10),
					card);
	e1000_reg_write(E1000_RDBAL, V2P(card->rbd[0]), card);
	e1000_reg_write(E1000_RDBAH, 0x00000000, card);
	e1000_reg_write(E1000_RDLEN, (E1000_RBD_SLOTS * 16) << 7, card);
	e1000_reg_write(E1000_RDH, 0x00000000, card);
	e1000_reg_write(E1000_RDT, 0x00000000, card);
	//enable interrupts
	e1000_reg_write(E1000_IMS, E1000_IMS_RXSEQ | E1000_IMS_RXO | E1000_IMS_RXT0 | E1000_IMS_TXQE, card);
	//Receive control Register.
	e1000_reg_write(E1000_RCTL,
					E1000_RCTL_EN |
						E1000_RCTL_BAM |
						E1000_RCTL_BSIZE | 0x00000008,//|
		//  E1000_RCTL_SECRC,
					card);
	cprintf("e1000:Interrupt enabled mask:0x%x\n", e1000_reg_read(E1000_IMS, card));
	//Register interrupt handler here...
	picenable(card->irq_line);
	ioapicenable(card->irq_line, 0);
	ioapicenable(card->irq_line, 1);

	return 0;
}

int e1000_net_init(void *state, void *hwaddr)
{
	netcard_t *card = (netcard_t *)state;
	e1000_t *e1000 = (e1000_t *)card->prvt;

}

int e1000_net_send(void *state, const void *data, int len)
{
	netcard_t *card = (netcard_t *)state;
	e1000_t *e1000 = (e1000_t *)card->prvt;
}

int e1000_net_recv(void *state, void *data, int len)
{
	netcard_t *card = (netcard_t *)state;
	e1000_t *e1000 = (e1000_t *)card->prvt;

}

