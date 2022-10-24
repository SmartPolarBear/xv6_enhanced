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

static inline int transmit_init(netcard_t *card)
{

}

static inline int receive_init(netcard_t *card)
{

}

static inline int desc_init(netcard_t *card)
{

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

	if (!desc_init(nic))
	{
		cprintf("e1000_nic_attach: desc_init failed");
		goto cleanup;
	}

	if (!transmit_init(nic))
	{
		cprintf("e1000_nic_attach: transmit_init failed");
		goto cleanup;
	}

	if (!receive_init(nic))
	{
		cprintf("e1000_nic_attach: receive_init failed");
		goto cleanup;
	}

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
	return len;
}

int e1000_net_recv(void *state, void *data, int len)
{
	netcard_t *card = (netcard_t *)state;
	e1000_t *e1000 = (e1000_t *)card->prvt;
	return len;
}

