//
// Created by bear on 10/19/2022.
//

#include "types.h"
#include "debug.h"
#include "defs.h"
#include "mmu.h"
#include "slab.h"
#include "mp.h"
#include "memlayout.h"

#include "pci.h"
#include "e1000_nic.h"
#include "net.h"

int e1000_net_init(void *state, void *hwaddr);
int e1000_open(void *state);
int e1000_stop(void *state);
int e1000_net_send(void *state, const void *data, int len);
int e1000_net_recv(void *state, void *data, int len);

netcard_opts_t e1000_opts = {
	.init = e1000_net_init,
	.open = e1000_open,
	.stop = e1000_stop,
	.send = e1000_net_send,
	.receive = e1000_net_recv,
};

typedef struct e1000
{
	struct TD tx_descs[E1000_TXDESC_LEN]__attribute__((aligned(16)));;
	struct RD rx_descs[E1000_RXDESC_LEN]__attribute__((aligned(16)));;

	struct packet *tbuf[E1000_TXDESC_LEN];
	struct packet *rbuf[E1000_RXDESC_LEN];

	volatile uint32 *mmio_base;
	size_t mmio_size;

	uint8 addr[6];
	uint32 irq;

} e1000_t;

kmem_cache_t *e1000_cache = NULL;
size_t e1000_count = 0;

static void pciw(e1000_t *card, int index, uint32 value)
{
	card->mmio_base[index / 4] = value;
	card->mmio_base[index / 4];
}

static uint32 pcir(e1000_t *card, int index)
{
	card->mmio_base[index / 4];
	return card->mmio_base[index / 4];
}

static uint16
e1000_eeprom_read(struct e1000 *dev, uint8 addr)
{
	uint32 eerd;
	pciw(dev, E1000_EERD, E1000_EERD_READ | addr << E1000_EERD_ADDR);
	while (!((eerd = pcir(dev, E1000_EERD)) & E1000_EERD_DONE))
		microdelay(1);
	return (uint16)(eerd >> E1000_EERD_DATA);
}

static void
e1000_read_addr_from_eeprom(struct e1000 *dev, uint8 *dst)
{
	uint16 data;
	for (int n = 0; n < 3; n++)
	{
		data = e1000_eeprom_read(dev, n);
		dst[n * 2 + 0] = (data & 0xff);
		dst[n * 2 + 1] = (data >> 8) & 0xff;
	}
}
static uintptr_t
e1000_resolve_mmio_base(struct pci_func *pcif)
{
	uint32 mmio_base = 0;
	for (int n = 0; n < 6; n++)
	{
		if (pcif->reg_base[n] > 0xffff)
		{
			KDEBUG_ASSERT(pcif->reg_size[n] == (1 << 17));
			mmio_base = pcif->reg_base[n];
			break;
		}
	}
	return mmio_base;
}

static void
e1000_rx_init(struct e1000 *dev)
{
	// setup rx descriptors
	uint64 base = (uint64)(V2P(dev->rx_descs));
	pciw(dev, E1000_RDBAL, (uint32)(base & 0xffffffff));
	pciw(dev, E1000_RDBAH, (uint32)(base >> 32));
	// rx descriptor lengh
	pciw(dev, E1000_RDLEN, (uint32)(E1000_RXDESC_LEN * sizeof(struct RD)));
	// setup head/tail
	pciw(dev, E1000_RDH, 0);
	pciw(dev, E1000_RDT, E1000_RXDESC_LEN - 1);
	// set tx control register
	pciw(dev, E1000_RCTL, (
		E1000_RCTL_SBP | /* store bad packet */
			E1000_RCTL_UPE | /* unicast promiscuous enable */
			E1000_RCTL_MPE | /* multicast promiscuous enab */
			E1000_RCTL_RDMTS_HALF | /* rx desc min threshold size */
			E1000_RCTL_SECRC | /* Strip Ethernet CRC */
			E1000_RCTL_LPE | /* long packet enable */
			E1000_RCTL_BAM | /* broadcast enable */
			E1000_RCTL_SZ_2048 | /* rx buffer size 2048 */
			0)
	);
}

static void
e1000_tx_init(struct e1000 *dev)
{
	// setup tx descriptors
	uint64 base = (uint64)(V2P(dev->tx_descs));
	pciw(dev, E1000_TDBAL, (uint32)(base & 0xffffffff));
	pciw(dev, E1000_TDBAH, (uint32)(base >> 32));
	// tx descriptor length
	pciw(dev, E1000_TDLEN, (uint32)(E1000_TXDESC_LEN * sizeof(struct TD)));
	// setup head/tail
	pciw(dev, E1000_TDH, 0);
	pciw(dev, E1000_TDT, 0);
	// set tx control register
	pciw(dev, E1000_TCTL, (
		E1000_TCTL_PSP | /* pad short packets */
			0)
	);
}

int e1000_open(void *state)
{
	netdev_t *netdev = (netdev_t *)state;
	struct e1000 *dev = (struct e1000 *)netdev->priv;
	// enable interrupts
	pciw(dev, E1000_IMS, E1000_IMS_RXT0);
	// clear existing pending interrupts
	pcir(dev, E1000_ICR);
	// enable RX/TX
	pciw(dev, E1000_RCTL, pcir(dev, E1000_RCTL) | E1000_RCTL_EN);
	pciw(dev, E1000_TCTL, pcir(dev, E1000_TCTL) | E1000_TCTL_EN);
	// link up
	pciw(dev, E1000_CTL, pcir(dev, E1000_CTL) | E1000_CTL_SLU);
	netdev->flags |= NETDEV_FLAG_UP;
	return 0;
}

int e1000_stop(void *state)
{
	netdev_t *netdev = (netdev_t *)state;
	struct e1000 *dev = (struct e1000 *)netdev->priv;
	// disable interrupts
	pciw(dev, E1000_IMC, E1000_IMS_RXT0);
	// clear existing pending interrupts
	pcir(dev, E1000_ICR);
	// disable RX/TX
	pciw(dev, E1000_RCTL, pcir(dev, E1000_RCTL) & ~E1000_RCTL_EN);
	pciw(dev, E1000_TCTL, pcir(dev, E1000_TCTL) & ~E1000_TCTL_EN);
	// link down
	pciw(dev, E1000_CTL, pcir(dev, E1000_CTL) & ~E1000_CTL_SLU);
	netdev->flags &= ~NETDEV_FLAG_UP;
	return 0;
}

void e1000_init(void)
{
	e1000_cache = kmem_cache_create("e1000", sizeof(e1000_t), 0);
	if (!e1000_cache)
	{
		panic("e1000_init: kmem_cache_create failed");
	}
}

static inline int desc_init(netdev_t *card)
{
	e1000_t *e1000 = (e1000_t *)card->priv;

	// two packet per page
	int i = 0;
	for (i = 0; i < E1000_TXDESC_LEN; i += 2)
	{
		char *p = page_alloc();
		if (!p)
		{
			cprintf("e1000: page_alloc failed for tbuf\n");
			goto fail_tbuf;
		}

		memset(p, 0, PGSIZE);
		e1000->tbuf[i] = (struct packet *)p;
		e1000->tbuf[i + 1] = (struct packet *)(p + PKTSIZE);
	}

	i = 0;
	for (i = 0; i < E1000_RXDESC_LEN; i += 2)
	{
		char *p = page_alloc();
		if (!p)
		{
			cprintf("e1000: page_alloc failed for rbuf\n");
			goto fail_rbuf;
		}

		memset(p, 0, PGSIZE);
		e1000->rbuf[i] = (struct packet *)p;
		e1000->rbuf[i + 1] = (struct packet *)(p + PKTSIZE);
	}

	// send
	for (i = 0; i < E1000_TXDESC_LEN; ++i)
	{
		e1000->tx_descs[i].addr = V2P(e1000->tbuf[i]->buf);
		e1000->tx_descs[i].length = PKTSIZE;
		e1000->tx_descs[i].status = E1000_TXD_STAT_DD;
		e1000->tx_descs[i].cmd = E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP;
	}

	// receive
	for (i = 0; i < E1000_RXDESC_LEN; ++i)
	{
		e1000->rx_descs[i].addr = V2P(e1000->rbuf[i]->buf);
	}

	return 0;

fail_rbuf:
	for (int j = 0; j < i; j += 2)
	{
		if (e1000->rbuf[j])
		{
			page_free((char *)e1000->rbuf[j]);
		}
	}
	i = E1000_TXDESC_LEN;
fail_tbuf:
	for (int j = 0; j < i; j += 2)
	{
		if (e1000->tbuf[j])
		{
			page_free((char *)e1000->tbuf[j]);
		}
	}

	return -1;
}

int e1000_nic_attach(struct pci_func *pcif)
{
	// Enable PCI function
	pci_func_enable(pcif);

	// Allocate card
	e1000_t *card = (e1000_t *)kmem_cache_alloc(e1000_cache);
	memset(card, 0, sizeof(e1000_t));

	uint32 id = e1000_count++;

	char name[8] = "e1000_";
	name[6] = '0' + id;

	netdev_t *nic = nic_register(name, pcif, &e1000_opts, card);

	if (desc_init(nic))
	{
		cprintf("e1000_nic_attach: desc_init failed\n");
		goto cleanup;
	}

	card->mmio_base = (volatile uint32 *)e1000_resolve_mmio_base(pcif);
	cprintf("e1000: base: 0x%x status reg: %x\n", card->mmio_base, pcir(card, E1000_STATUS));

	// Read HW address from EEPROM
	e1000_read_addr_from_eeprom(card, card->addr);
	cprintf("e1000: macaddr: %x:%x:%x:%x:%x:%x\n",
			card->addr[0],
			card->addr[1],
			card->addr[2],
			card->addr[3],
			card->addr[4],
			card->addr[5]);
	// Register I/O APIC
	card->irq = pcif->irq_line;
	ioapicenable(card->irq, ncpu - 1);

	// Initialize Multicast Table Array
	for (int n = 0; n < 128; n++)
		pciw(card, E1000_MTA + (n << 2), 0);
	// Initialize RX/TX
	e1000_rx_init(card);
	e1000_tx_init(card);

	// Alloc netdev
	nic->flags |= NETDEV_FLAG_RUNNING;

	cprintf("e1000: init done\n");

	return TRUE;

cleanup:
	nic = nic_unregister(nic);
	nic_free(nic);
	kmem_cache_free(e1000_cache, card);
	return FALSE;
}

int e1000_net_init(void *state, void *hwaddr)
{
	netdev_t *card = (netdev_t *)state;
	e1000_t *e1000 = (e1000_t *)card->priv;
	memmove(hwaddr, e1000->addr, 6);

	return 0;
}

int e1000_net_send(void *state, const void *data, int len)
{
	netdev_t *card = (netdev_t *)state;
	e1000_t *e1000 = (e1000_t *)card->priv;
	uint32 tail = pcir(e1000, E1000_TDT);
	struct TD *next_desc = &e1000->tx_descs[tail];

	if (!(next_desc->status & E1000_TXD_STAT_DD) && (next_desc->cmd & E1000_TXD_CMD_RS))
	{
		return -1;
	}

	if (len > PKTSIZE)
	{
		len = PKTSIZE;
	}

	memmove(e1000->tbuf[tail]->buf, data, len);
	next_desc->length = len;
	next_desc->status = 0;
	next_desc->cmd = (E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP);

	pciw(e1000, E1000_TDT, (tail + 1) % E1000_TXDESC_LEN);
	while (!(next_desc->status & 0x0f))
	{
		microdelay(1);
	}
	return len;
}

int e1000_net_recv(void *state, void *data, int len)
{
	netdev_t *card = (netdev_t *)state;
	e1000_t *e1000 = (e1000_t *)card->priv;

	uint32 tail = (pcir(e1000, E1000_RDT) + 1) % E1000_RXDESC_LEN;
	struct RD *next_desc = &e1000->rx_descs[tail];

	if (!(next_desc->status & E1000_RXD_STAT_DD))
	{
		return -1;
	}

	if (next_desc->length < 60)
	{
		cprintf("e1000: short packet (%d bytes)\n", next_desc->length);
		return -1;
	}
	if (!(next_desc->status & E1000_RXD_STAT_EOP))
	{
		cprintf("e1000: not EOP! this driver does not support packet that do not fit in one buffer\n");
		return -1;

	}

	if (next_desc->errors)
	{
		cprintf("e1000: rx errors (0x%x)\n", next_desc->errors);
		return -1;
	}

	if (next_desc->length < len)
	{
		len = next_desc->length;
	}

	memmove(data, e1000->rbuf[tail]->buf, len);
	next_desc->status = 0;
	pciw(e1000, E1000_RDT, tail);
	return next_desc->length;
}

