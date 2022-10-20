//
// Created by bear on 10/19/2022.
//

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "lwip/dhcp.h"
#include "lwip/etharp.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"

struct netif netif[NNETIF];

err_t
linkoutput(struct netif *netif, struct pbuf *p)
{
//	int n = (uintptr_t)netif->state;
//	struct pbuf *q;
//
//	for (q = p; q; q = q->next) {
//		if(virtio_net_send(n, q->payload, q->len))
//			return ERR_IF;
//	}

	return ERR_OK;
}

int
linkinput(struct netif *netif)
{
//	int n = (uintptr_t)netif->state, len;
//	struct pbuf *p;
//
//	p = pbuf_alloc(PBUF_RAW, 1514, PBUF_RAM);
//	if(!p)
//		return 0;
//
//	len = virtio_net_recv(n, p->payload, p->len);
//
//	if(len > 0){
//		/* shrink pbuf to actual size */
//		pbuf_realloc(p, len);
//
//		if(netif->input(p, netif) == ERR_OK)
//			return len;
//
//		printf("linkinput: drop packet (%d bytes)\n", len);
//	}
//
//	pbuf_free(p);
//	return len;
	return 0;
}

err_t
linkinit(struct netif *netif)
{
//	int n = (uintptr_t)netif->state;
//
//	if (virtio_net_init(n, &netif->hwaddr))
//	{
//		return ERR_IF;
//	}
//
//	netif->hwaddr_len = ETH_HWADDR_LEN;
//	netif->linkoutput = linkoutput;
//	netif->output = etharp_output;
//	netif->mtu = 1500;
//	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;

	return ERR_OK;
}

void
netadd(int n)
{
//	struct netif *new = &netif[n];
//	int i;
//	char addr[IPADDR_STRLEN_MAX], netmask[IPADDR_STRLEN_MAX], gw[IPADDR_STRLEN_MAX];
//
//	if (!netif_add_noaddr(new, (void *)(uintptr_t)n, linkinit, netif_input))
//	{
//		panic("netadd");
//	}
//
//	new->name[0] = 'e';
//	new->name[1] = 'n';
//	netif_set_link_up(new);
//	netif_set_up(new);
//
//	printf("net %d: mac ", n);
//	for (i = 0; i < ETH_HWADDR_LEN; ++i)
//	{
//		if (i)
//		{
//			printf(":");
//		}
//		if (new->hwaddr[i] < 0x10)
//		{
//			printf("0");
//		}
//		printf("%x", new->hwaddr[i]);
//	}
//	printf("\n");
//
//	dhcp_start(new);
//	/* wait until DHCP succeeds */
//	while (!dhcp_supplied_address(new))
//	{
//		linkinput(new);
//		sys_check_timeouts();
//	}
//
//	ipaddr_ntoa_r(netif_ip_addr4(new), addr, sizeof(addr));
//	ipaddr_ntoa_r(netif_ip_netmask4(new), netmask, sizeof(netmask));
//	ipaddr_ntoa_r(netif_ip_gw4(new), gw, sizeof(gw));
//	printf("net %d: addr %s netmask %s gw %s\n", n, addr, netmask, gw);
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
	lwip_init();
	netadd(0);
	netif_set_default(&netif[0]);
}

uint32
sys_now(void)
{
	/* good enough for qemu */
	return r_mtime() / 10000;
}

unsigned long
r_mtime(void)
{
//	return *(uint64 *)CLINT_MTIME;
	return -1;
}
