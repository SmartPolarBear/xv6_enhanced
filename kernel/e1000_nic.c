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

	volatile uint32 *mmio_e1000;
	size_t mmio_size;
} e1000_t;

kmem_cache_t *e1000_cache = NULL;

static void
pciw(e1000_t *c, int index, int value)
{
	c->mmio_e1000[index] = value;
	c->mmio_e1000[index];
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

	card->mmio_e1000 = (uint32 *)mmio_base;
	card->mmio_size = mmio_size;

	nic_register("e1000", pcif, &e1000_opts, card);

	// Initialize the NIC
//	e1000_nic_init(mmio);

	return 0;
}

int e1000_net_init(void *state, void *hwaddr)
{

}

int e1000_net_send(void *state, const void *data, int len)
{

}

int e1000_net_recv(void *state, void *data, int len)
{

}

