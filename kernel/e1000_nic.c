//
// Created by bear on 10/19/2022.
//

#include "types.h"
#include "debug.h"
#include "defs.h"
#include "mmu.h"
#include "slab.h"
#include "x86.h"
#include "memlayout.h"

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
	struct TD *transmit_desc_list; // 32
	struct RD *receive_desc_list; // 128

	struct packet *tbuf[32];
	struct packet *rbuf[128];

	volatile uint32 *mmio_base;
	size_t mmio_size;
} e1000_t;

kmem_cache_t *e1000_cache = NULL;
size_t e1000_count = 0;

static inline void pciw(e1000_t *card, int index, uint32 value)
{
	card->mmio_base[index] = value;
	card->mmio_base[index];
}

static inline uint32 pcir(e1000_t *card, int index)
{
	card->mmio_base[index];
	return card->mmio_base[index];
}

void e1000_init(void)
{
	e1000_cache = kmem_cache_create("e1000", sizeof(e1000_t), 0);
	if (!e1000_cache)
	{
		panic("e1000_init: kmem_cache_create failed");
	}
}

static inline void transmit_init(netcard_t *card)
{
	e1000_t *e1000 = (e1000_t *)card->prvt;

	//TD Base Address register
	pciw(e1000, E1000_TDBAL, V2P(e1000->transmit_desc_list));
	pciw(e1000, E1000_TDBAH, 0);

	//TD Descriptor Length register
	pciw(e1000, E1000_TDLEN, 32 * sizeof(struct TD));

	//TD head and tail register
	pciw(e1000, E1000_TDH, 0x0);
	pciw(e1000, E1000_TDT, 0x0);

	//TD control register
	pciw(e1000, E1000_TCTL, TCTL_EN | TCTL_PSP | (TCTL_CT & (0x10 << 4)) | (TCTL_COLD & (0x40 << 12)));

	//Transmit Inter Packets Gap register
	pciw(e1000, E1000_TIPG, 10 | (8 << 10) | (12 << 20));
}

static inline void receive_init(netcard_t *card)
{
	e1000_t *e1000 = (e1000_t *)card->prvt;
	//Receive Address Register
	//pciw(E1000_RA, 0x52540012);
	pciw(e1000, E1000_RA, 0x12005452);
	pciw(e1000, E1000_RA + 1, 0x5634 | E1000_RAV);

	//Multicast Table Array
	for (int i = 0; i < 128; i++)
		pciw(e1000, E1000_MTA + i, 0);
	//Base Address register
	pciw(e1000, E1000_RDBAL, V2P(e1000->receive_desc_list));
	pciw(e1000, E1000_RDBAH, 0);

	// Descriptor Length
	pciw(e1000, E1000_RDLEN, sizeof(struct RD) * 128);

	// head and tail register
	pciw(e1000, E1000_RDH, 0);
	pciw(e1000, E1000_RDT, 128 - 1);

	pciw(e1000, E1000_RCTL, RCTL_EN | RCTL_LPE | RCTL_LBM_NO | RCTL_SZ_2048 | RCTL_SECRC);

}

static inline int desc_init(netcard_t *card)
{
	e1000_t *e1000 = (e1000_t *)card->prvt;
	// TD=384, RD=1536, 1 packet =2048

	// TD and RD use a page. waste some bytes
	char *td_rd = page_alloc();
	if (!td_rd)
	{
		goto fail;
	}

	e1000->transmit_desc_list = (struct TD *)td_rd;
	e1000->receive_desc_list = (struct RD *)(td_rd + 2048);

	// two packet per page
	int i = 0;
	for (i = 0; i < 32; i += 2)
	{
		char *p = page_alloc();
		if (!p)
		{
			cprintf("e1000: page_alloc failed for tbuf\n");
			goto fail_tbuf;
		}

		e1000->tbuf[i] = (struct packet *)p;
		e1000->tbuf[i + 1] = (struct packet *)(p + PKTSIZE);
	}

	i = 0;
	for (i = 0; i < 128; i += 2)
	{
		char *p = page_alloc();
		if (!p)
		{
			cprintf("e1000: page_alloc failed for rbuf\n");
			goto fail_rbuf;
		}

		e1000->rbuf[i] = (struct packet *)p;
		e1000->rbuf[i + 1] = (struct packet *)(p + PKTSIZE);
	}

	// setup descs

	// send
	for (i = 0; i < 32; ++i)
	{
		memset(&e1000->transmit_desc_list[i], 0, sizeof(struct TD));
		e1000->transmit_desc_list[i].addr = V2P(&e1000->tbuf[i]);
		e1000->transmit_desc_list[i].status = TXD_STAT_DD;
		e1000->transmit_desc_list[i].cmd = TXD_CMD_RS | TXD_CMD_EOP;
	}

	// receive
	for (i = 0; i < 128; ++i)
	{
		memset(&e1000->receive_desc_list[i], 0, sizeof(struct RD));
		e1000->receive_desc_list[i].addr = V2P(&e1000->rbuf[i]);
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
	i = 32;
fail_tbuf:
	for (int j = 0; j < i; j += 2)
	{
		if (e1000->tbuf[j])
		{
			page_free((char *)e1000->tbuf[j]);
		}
	}
	page_free(td_rd);
fail:
	return -1;
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

	netcard_t *nic = nic_register(name, pcif, &e1000_opts, card);

	if (desc_init(nic))
	{
		cprintf("e1000_nic_attach: desc_init failed\n");
		goto cleanup;
	}

	transmit_init(nic);
	receive_init(nic);

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
	netcard_t *card = (netcard_t *)state;
	e1000_t *e1000 = (e1000_t *)card->prvt;
	return 0;
}

int e1000_net_send(void *state, const void *data, int len)
{
	netcard_t *card = (netcard_t *)state;
	e1000_t *e1000 = (e1000_t *)card->prvt;
	uint32 tail = pcir(e1000, E1000_TDT);
	struct TD *next_desc = &e1000->transmit_desc_list[tail];

	if ((next_desc->status & TXD_STAT_DD) != TXD_STAT_DD)
	{
		return -1;
	}
	if (len > PKTSIZE)
	{
		len = PKTSIZE;
	}

	memmove(&e1000->tbuf[tail], data, len);
	next_desc->length = len;
	next_desc->status &= !TXD_STAT_DD;
	pciw(e1000, E1000_TDT, (tail + 1) % 32);
	//cprintf("send : %s\n", addr);
	return 0;
}

int e1000_net_recv(void *state, void *data, int len)
{
	netcard_t *card = (netcard_t *)state;
	e1000_t *e1000 = (e1000_t *)card->prvt;

	uint32 tail = (pcir(e1000, E1000_RDT) + 1) % 128;
	struct RD *next_desc = &e1000->receive_desc_list[tail];

	if ((next_desc->status & RXD_STAT_DD) != RXD_STAT_DD)
	{
		return -1;
	}

	//cprintf("tail value :%d\n", tail) ;
	//cprintf("buflen %d desclen %d\n", len, next_desc->length);
	if (next_desc->length < len)
	{
		len = next_desc->length;
	}

	memmove(data, &e1000->rbuf[tail], len);
	next_desc->status &= !RXD_STAT_DD;
	pciw(e1000, E1000_RDT, tail);
	return next_desc->length;
}

