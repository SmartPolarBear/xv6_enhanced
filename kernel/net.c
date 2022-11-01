//
// Created by bear on 10/19/2022.
//

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "date.h"
#include "list.h"
#include "slab.h"
#include "net.h"

#include "lwip/dhcp.h"
#include "lwip/etharp.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"

struct netif netif[NNETIF];

list_head_t netcard_list = LIST_HEAD_INIT(netcard_list);
size_t netcard_count = 0;

kmem_cache_t *netcard_cache = NULL;
spinlock_t netcard_list_lock;

netdev_t *find_card_by_id(int n)
{
	list_head_t *pos;
	acquire(&netcard_list_lock);

	list_for_each(pos, &netcard_list)
	{
		netdev_t *card = list_entry(pos, netdev_t, link);
		if (card->id == n)
		{
			release(&netcard_list_lock);
			return card;
		}
	}

	release(&netcard_list_lock);
	return NULL;
}

netdev_t *find_card_by_name(char *name)
{
	list_head_t *pos;
	acquire(&netcard_list_lock);

	list_for_each(pos, &netcard_list)
	{
		netdev_t *card = list_entry(pos, netdev_t, link);
		if (strncmp(card->name, name, NNETCARDNAME) == 0)
		{
			release(&netcard_list_lock);
			return card;
		}
	}

	release(&netcard_list_lock);
	return NULL;
}

err_t
linkoutput(struct netif *netif, struct pbuf *p)
{
	netdev_t *card = netif->state;
	struct pbuf *q;

	if (!card->opts->send)
	{
		return ERR_IF;
	}

	for (q = p; q; q = q->next)
	{
		if (card->opts->send(card, q->payload, q->len))
		{
			return ERR_IF;
		}
	}

	return ERR_OK;
}

int
linkinput(struct netif *netif)
{
	int len;
	struct pbuf *p;

	p = pbuf_alloc(PBUF_RAW, 1514, PBUF_RAM);
	if (!p)
	{
		return 0;
	}

	netdev_t *card = netif->state;
	if (!card->opts->receive)
	{
		return 0;
	}

	len = card->opts->receive(card, p->payload, p->len);

	if (len > 0)
	{
		/* shrink pbuf to actual size */
		pbuf_realloc(p, len);

		if (netif->input(p, netif) == ERR_OK)
		{
			return len;
		}

		cprintf("linkinput: drop packet (%d bytes)\n", len);
	}

	pbuf_free(p);
	return len;
}

err_t
linkinit(struct netif *netif)
{
	netdev_t *card = netif->state;

	if (!card->opts->init)
	{
		return ERR_IF;
	}

	if (card->opts->init(card, &netif->hwaddr))
	{
		return ERR_IF;
	}

	netif->hwaddr_len = ETH_HWADDR_LEN;
	netif->linkoutput = linkoutput;
	netif->output = etharp_output;
	netif->mtu = 1500;
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;

	return ERR_OK;
}

void
netadd(int n)
{
	struct netif *new = &netif[n];
	netdev_t *card = find_card_by_id(n);

	int i;
	char addr[IPADDR_STRLEN_MAX], netmask[IPADDR_STRLEN_MAX], gw[IPADDR_STRLEN_MAX];

	if (!netif_add_noaddr(new, (void *)card, linkinit, netif_input))
	{
		panic("netadd");
	}

	new->name[0] = 'e';
	new->name[1] = 'n';

	netif_set_link_up(new);
	netif_set_up(new);

	cprintf("net %d: mac ", n);
	for (i = 0; i < ETH_HWADDR_LEN; ++i)
	{
		if (i)
		{
			cprintf(":");
		}
		if (new->hwaddr[i] < 0x10)
		{
			cprintf("0");
		}
		cprintf("%x", new->hwaddr[i]);
	}
	cprintf("\n");

	if (!card->opts->open || card->opts->open(card))
	{
		panic("netadd： cannot open device\n");
	}

	dhcp_start(new);
	/* wait until DHCP succeeds */
	while (!dhcp_supplied_address(new))
	{
		linkinput(new);
		sys_check_timeouts();
	}

	ipaddr_ntoa_r(netif_ip_addr4(new), addr, sizeof(addr));
	ipaddr_ntoa_r(netif_ip_netmask4(new), netmask, sizeof(netmask));
	ipaddr_ntoa_r(netif_ip_gw4(new), gw, sizeof(gw));
	cprintf("net %d: addr %s netmask %s gw %s\n", n, addr, netmask, gw);
}

int
nettimer(void)
{
	sys_check_timeouts();
	return linkinput(&netif[0]);
}

void
netinit(void)
{
	list_init(&netcard_list);
	initlock(&netcard_list_lock, "netcard_list");

	netcard_cache = kmem_cache_create("netcard_cache", sizeof(struct netdev), 0);
	if (!netcard_cache)
	{
		panic("netinit: kmem_cache_create");
	}

}

netdev_t *nic_register(char *name, struct pci_func *pcif, struct netcard_opts *opts, void *prvt)
{
	struct netdev *card = kmem_cache_alloc(netcard_cache);
	if (!card)
	{
		panic("nic_register: kmem_cache_alloc");
	}

	memset(card, 0, sizeof(struct netdev));
	strncpy(card->name, name, sizeof(card->name));

	card->func = pcif;
	card->opts = opts;
	card->priv = prvt;
	card->id = netcard_count++;

	acquire(&netcard_list_lock);
	list_add(&card->link, &netcard_list);
	release(&netcard_list_lock);

	cprintf("nic_register: %s registered\n", card->name);
	return card;
}

struct netdev *nic_unregister(struct netdev *nic)
{
	acquire(&netcard_list_lock);
	list_del(&nic->link);
	release(&netcard_list_lock);

	return nic;
}

void nic_free(struct netdev *nic)
{
	kmem_cache_free(netcard_cache, nic);
}

void netstart(void)
{
	if (list_empty(&netcard_list))
	{
		cprintf("netstart: no netdev\n");
		return;
	}

	lwip_init();
	netadd(0);
	netif_set_default(&netif[0]);
}



