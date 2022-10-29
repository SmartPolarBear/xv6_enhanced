//
// Created by bear on 10/29/2022.
//
#include "types.h"
#include "defs.h"
#include "slab.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "param.h"
#include "fs.h"
#include "file.h"

#include "socket.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"

kmem_cache_t *socket_cache;

void socketinit(void)
{
	socket_cache = kmem_cache_create("socket_cache", sizeof(struct socket), 0);
	if (socket_cache == NULL)
	{
		panic("socketinit: socket_cache");
	}
}

struct file *socketalloc(int domain, int type, int protocol)
{
	if (domain != AF_INET)
	{
		return NULL;
	}

	if (type != SOCK_STREAM && type != SOCK_DGRAM)
	{
		return NULL;
	}

	if (protocol != IPPROTO_TCP && protocol != IPPROTO_UDP)
	{
		return NULL;
	}

	file_t *file = filealloc();
	if (file == NULL)
	{
		return NULL;
	}

	socket_t *socket = kmem_cache_alloc(socket_cache);
	if (socket == NULL)
	{
		fileclose(file);
		return NULL;
	}

	socket->type = type;

	if (type == SOCK_STREAM)
	{
		socket->desc = (int)tcp_new_ip_type(IPADDR_TYPE_ANY);
	}
	else
	{
		socket->desc = (int)udp_new_ip_type(IPADDR_TYPE_ANY);
	}

	file->type = FD_SOCKET;
	file->socket = socket;
	file->readable = TRUE;
	file->writable = TRUE;

	return file;
}

void socketclose(struct socket *skt)
{
	if (skt->type == SOCK_STREAM)
	{
		tcp_close((struct tcp_pcb *)skt->desc);
	}
	else
	{
		udp_remove((struct udp_pcb *)skt->desc);
	}
}

int socketconnect(struct socket *skt, struct sockaddr *addr, int addr_len)
{
	if (skt->type == SOCK_STREAM)
	{
		struct tcp_pcb *pcb = (struct tcp_pcb *)skt->desc;
		return tcp_connect(pcb, &addr->sin_addr, addr->sin_port, NULL);
	}
	else
	{
		struct udp_pcb *pcb = (struct udp_pcb *)skt->desc;
		udp_connect(pcb, &addr->sin_addr, addr->sin_port);
	}
}

int socketbind(struct socket *skt, struct sockaddr *addr, int addr_len)
{
	if (skt->type == SOCK_STREAM)
	{
		struct tcp_pcb *pcb = (struct tcp_pcb *)skt->desc;
		return tcp_bind(pcb, &addr->sin_addr, addr->sin_port);
	}
	else
	{
		struct udp_pcb *pcb = (struct udp_pcb *)skt->desc;
		return udp_bind(pcb, &addr->sin_addr, addr->sin_port);
	}
}

int socketlisten(struct socket *skt, int backlog)
{
	if (skt->type == SOCK_STREAM)
	{
		struct tcp_pcb *pcb = (struct tcp_pcb *)skt->desc;
		return tcp_listen(pcb);
	}
	else
	{
		return -1;
	}
}

struct file *socketaccept(struct socket *skt, struct sockaddr *addr, int *addr_len)
{
	if (skt->type != SOCK_STREAM)
	{
		return NULL;
	}
	struct tcp_pcb *pcb = (struct tcp_pcb *)skt->desc;
	struct tcp_pcb *newpcb = tcp_accept(pcb);
	if (newpcb == NULL)
	{
		return NULL;
	}

	file_t *file = filealloc();
	if (file == NULL)
	{
		return NULL;
	}

	socket_t *socket = kmem_cache_alloc(socket_cache);
	if (socket == NULL)
	{
		fileclose(file);
		return NULL;
	}

	socket->type = SOCK_STREAM;
	socket->desc = (int)newpcb;

	file->type = FD_SOCKET;
	file->socket = socket;
	file->readable = TRUE;
	file->writable = TRUE;

	return file;

}

int socketread(struct socket *skt, char *buf, int len)
{
	if (skt->type == SOCK_STREAM)
	{
		struct tcp_pcb *pcb = (struct tcp_pcb *)skt->desc;
		return tcp_recv(pcb, buf, len, 0);
	}
	else
	{
		struct udp_pcb *pcb = (struct udp_pcb *)skt->desc;
		return udp_recv(pcb, buf, len, 0);
	}
}

int socketwrite(struct socket *skt, char *buf, int len)
{
	if (skt->type == SOCK_STREAM)
	{
		struct tcp_pcb *pcb = (struct tcp_pcb *)skt->desc;
		return tcp_write(pcb, buf, len, 0);
	}
	else
	{
		struct udp_pcb *pcb = (struct udp_pcb *)skt->desc;
		return udp_send(pcb, buf, len);
	}
}

int socketrecvfrom(struct socket *s, char *buf, int len, struct sockaddr *addr, int *addrlen)
{
	if (s->type != SOCK_DGRAM)
	{
		return -1;
	}

	struct udp_pcb *pcb = (struct udp_pcb *)s->desc;
	return udp_recvfrom(pcb, buf, len, addr, addrlen);
}

int socketsendto(struct socket *skt, char *buf, int len, struct sockaddr *addr, int addr_len)
{
	if (skt->type != SOCK_DGRAM)
	{
		return -1;
	}

	struct udp_pcb *pcb = (struct udp_pcb *)skt->desc;
	return udp_sendto(pcb, buf, len, addr, addr_len);
}

int socketioctl(struct socket *skt, int req, void *arg)
{
	struct ifreq *ifreq;
	struct netdev *dev;
	struct netif *iface;

	switch (req)
	{
	case SIOCGIFINDEX:
		ifreq = (struct ifreq *)arg;
		dev = netdev_by_name(ifreq->ifr_name);
		if (!dev)
		{
			return -1;
		}
		ifreq->ifr_ifindex = dev->index;
		break;
	case SIOCGIFNAME:
		ifreq = (struct ifreq *)arg;
		dev = netdev_by_index(ifreq->ifr_ifindex);
		if (!dev)
		{
			return -1;
		}
		strncpy(ifreq->ifr_name, dev->name, sizeof(ifreq->ifr_name));
		break;
	case SIOCSIFNAME:
		/* TODO */
		break;
	case SIOCGIFHWADDR:
		ifreq = (struct ifreq *)arg;
		dev = netdev_by_name(ifreq->ifr_name);
		if (!dev)
		{
			return -1;
		}
		/* TODO: HW type check */
		memcpy(ifreq->ifr_hwaddr.sa_data, dev->addr, dev->alen);
		break;
	case SIOCSIFHWADDR:
		/* TODO */
		break;
	case SIOCGIFFLAGS:
		ifreq = (struct ifreq *)arg;
		dev = netdev_by_name(ifreq->ifr_name);
		if (!dev)
		{
			return -1;
		}
		ifreq->ifr_flags = dev->flags;
		break;
	case SIOCSIFFLAGS:
		ifreq = (struct ifreq *)arg;
		dev = netdev_by_name(ifreq->ifr_name);
		if (!dev)
		{
			return -1;
		}
		if ((dev->flags & IFF_UP) != (ifreq->ifr_flags & IFF_UP))
		{
			if (ifreq->ifr_flags & IFF_UP)
			{
				dev->ops->open(dev);
			}
			else
			{
				dev->ops->stop(dev);
			}
		}
		break;
	case SIOCGIFADDR:
		ifreq = (struct ifreq *)arg;
		dev = netdev_by_name(ifreq->ifr_name);
		if (!dev)
		{
			return -1;
		}
		iface = netdev_get_netif(dev, ifreq->ifr_addr.sa_family);
		if (!iface)
		{
			return -1;
		}
		((struct sockaddr_in *)&ifreq->ifr_addr)->sin_addr = ((struct netif_ip *)iface)->unicast;
		break;
	case SIOCSIFADDR:
		ifreq = (struct ifreq *)arg;
		dev = netdev_by_name(ifreq->ifr_name);
		if (!dev)
		{
			return -1;
		}
		iface = netdev_get_netif(dev, ifreq->ifr_addr.sa_family);
		if (iface)
		{
			if (ip_netif_reconfigure(iface,
									 ((struct sockaddr_in *)&ifreq->ifr_addr)->sin_addr,
									 ((struct netif_ip *)iface)->netmask,
									 ((struct netif_ip *)iface)->gateway) == -1)
			{
				return -1;
			}
		}
		else
		{
			iface = ip_netif_alloc(((struct sockaddr_in *)&ifreq->ifr_addr)->sin_addr, 0xffffffff, 0);
			if (!iface)
			{
				return -1;
			}
			netdev_add_netif(dev, iface);
		}
		break;
	case SIOCGIFNETMASK:
		ifreq = (struct ifreq *)arg;
		dev = netdev_by_name(ifreq->ifr_name);
		if (!dev)
		{
			return -1;
		}
		iface = netdev_get_netif(dev, ifreq->ifr_addr.sa_family);
		if (!iface)
		{
			return -1;
		}
		((struct sockaddr_in *)&ifreq->ifr_netmask)->sin_addr = ((struct netif_ip *)iface)->netmask;
		break;
	case SIOCSIFNETMASK:
		ifreq = (struct ifreq *)arg;
		dev = netdev_by_name(ifreq->ifr_name);
		if (!dev)
		{
			return -1;
		}
		iface = netdev_get_netif(dev, ifreq->ifr_addr.sa_family);
		if (!iface)
		{
			return -1;
		}
		if (ip_netif_reconfigure(iface,
								 ((struct netif_ip *)iface)->unicast,
								 ((struct sockaddr_in *)&ifreq->ifr_addr)->sin_addr,
								 ((struct netif_ip *)iface)->gateway) == -1)
		{
			return -1;
		}
		break;
	case SIOCGIFBRDADDR:
		ifreq = (struct ifreq *)arg;
		dev = netdev_by_name(ifreq->ifr_name);
		if (!dev)
		{
			return -1;
		}
		iface = netdev_get_netif(dev, ifreq->ifr_addr.sa_family);
		if (!iface)
		{
			return -1;
		}
		((struct sockaddr_in *)&ifreq->ifr_broadaddr)->sin_addr = ((struct netif_ip *)iface)->broadcast;
		break;
	case SIOCSIFBRDADDR:
		/* TODO */
		break;
	case SIOCGIFMTU:
		ifreq = (struct ifreq *)arg;
		dev = netdev_by_name(ifreq->ifr_name);
		if (!dev)
		{
			return -1;
		}
		ifreq->ifr_mtu = dev->mtu;
		break;
	case SIOCSIFMTU:
		break;
	default:
		return -1;
	}
	return 0;
}