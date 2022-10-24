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

typedef struct e1000_card
{
	struct TD *transmit_desc_list[32];
	struct RD *receive_desc_list[128];

	struct packet *tbuf[32];
	struct packet *rbuf[128];

	uint32 id;
	volatile uint32 *mmio_e1000;
	size_t mmio_size;

	list_head_t link;
} e1000_card_t;

list_head_t e1000_cards = LIST_HEAD_INIT(e1000_cards);
size_t e1000_cards_count = 0;
kmem_cache_t *e1000_card_cache = NULL;

static void
pciw(e1000_card_t *c, int index, int value)
{
	c->mmio_e1000[index] = value;
	c->mmio_e1000[index];
}

static e1000_card_t *find_card(int id)
{
	list_head_t *pos = NULL;
	list_for_each(pos, &e1000_cards)
	{
		e1000_card_t *c = list_entry(pos, e1000_card_t, link);
		if (c->id == id)
		{
			return c;
		}
	}

	return NULL;
}

void e1000_init(void)
{
	list_init(&e1000_cards);
	e1000_card_cache = kmem_cache_create("e1000_card_cache", sizeof(e1000_card_t), 0);
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
	e1000_card_t *card = (e1000_card_t *)kmem_cache_alloc(e1000_card_cache);
	memset(card, 0, sizeof(e1000_card_t));

	card->id = e1000_cards_count++;
	card->mmio_e1000 = (uint32 *)mmio_base;
	card->mmio_size = mmio_size;

	list_add(&card->link, &e1000_cards);

	// Initialize the NIC
//	e1000_nic_init(mmio);

	return 0;
}

int e1000_net_init(int id, void *hwaddr)
{

}

int e1000_net_send(int id, const void *data, int len)
{

}

int e1000_net_recv(int id, void *data, int len)
{

}
