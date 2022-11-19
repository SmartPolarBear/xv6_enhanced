//
// Created by bear on 11/19/2022.
//

#include "types.h"
#include "errno.h"
#include "defs.h"
#include "slab.h"
#include "debug.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "param.h"
#include "file.h"
#include "mmu.h"
#include "proc.h"

#include "net.h"
#include "socket.h"
#include "internal/socket.h"

#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/raw.h"

static inline void lwip_udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
										  const ip_addr_t *addr, u16_t port)
{
	socket_t *socket = (socket_t *)arg;
	socket->recvfrom_params.recv_addr.sin_addr.addr = addr->addr;
	socket->recvfrom_params.recv_addr.sin_port = port;
	socket->recvfrom_params.recv_addr.sin_family = AF_INET;

	if (!p || p->tot_len > socket->recvfrom_params.recv_len)
	{
		if (p)
		{
			netbegin_op();
			pbuf_free(p);
			netend_op();
		}
		socket->recv_closed = TRUE;
		return;
	}
	// buffer hasn't been received
	if (socket->recv_buf)
	{
		return;
	}

	socket->recv_buf = p;
	socket->recv_offset = 0;

	socket->wakeup_retcode = 0;
	wakeup(&socket->recv_chan);
}

int udpalloc(struct socket *s)
{
	netbegin_op();
	s->pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
	if (s->pcb == NULL)
	{
		netend_op();
		return -ENOMEM;
	}
	netend_op();
	return 0;
}

int udpconnect(struct socket *s, struct sockaddr *addr, int addrlen)
{
	return -EPROTONOSUPPORT;
}

int udpbind(struct socket *s, struct sockaddr *addr, int addrlen)
{
	sockaddr_in_t *addr_in = (sockaddr_in_t *)addr;
	addrin_byteswap(addr_in);

	netbegin_op();
	struct udp_pcb *pcb = (struct udp_pcb *)s->pcb;
	err_t err = udp_bind(pcb, (ip_addr_t *)&addr_in->sin_addr, addr_in->sin_port);
	netend_op();
	if (err == ERR_OK)
	{
		return 0;
	}
	else if (err == ERR_USE)
	{
		return -EADDRINUSE;
	}
	return -EINVAL;
}

int udplisten(struct socket *s, int backlog)
{
	return -EPROTONOSUPPORT;
}

int udpaccept(struct socket *s, struct socket *newsock, struct sockaddr *addr, int *addrlen)
{
	return -EPROTONOSUPPORT;
}

int udpsend(struct socket *s, void *buf, int len, int flags)
{
	struct udp_pcb *pcb = (struct udp_pcb *)s->pcb;
	if (!pcb)
	{
		return -ECONNRESET;
	}

	netbegin_op();
	struct pbuf *pb = pbuf_alloc(PBUF_IP, len, PBUF_RAM);
	err_t err = pbuf_take(pb, buf, len);
	if (err != ERR_OK)
	{
		pbuf_free(pb);
		netend_op();
		return -ENOMEM;
	}

	err = udp_send(pcb, pb);
	if (err != ERR_OK)
	{
		pbuf_free(pb);
		netend_op();
		return -ECONNABORTED;
	}

	pbuf_free(pb);
	netend_op();
	return -ECONNABORTED;
}

int udprecv(struct socket *s, void *buf, int len, int flags)
{
	return -EPROTONOSUPPORT;
}

int udpsendto(struct socket *s, void *buf, int len, int flags, struct sockaddr *addr, int addrlen)
{
	struct sockaddr_in *in_addr = (struct sockaddr_in *)addr;
	addrin_byteswap(in_addr);
	if (in_addr->sin_family != AF_INET)
	{
		return -EAFNOSUPPORT;
	}

	netbegin_op();
	struct udp_pcb *pcb = (struct udp_pcb *)s->pcb;
	if (!pcb)
	{
		return -ECONNRESET;
	}
	struct pbuf *pb = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
	if (!pb)
	{
		netend_op();
		return -ENOMEM;
	}

	err_t err = pbuf_take(pb, buf, len);
	if (err != ERR_OK)
	{
		pbuf_free(pb);
		netend_op();
		return -ENOMEM;
	}

	err = udp_sendto(pcb, pb, &in_addr->sin_addr, in_addr->sin_port);
	if (err == ERR_OK)
	{
		pbuf_free(pb);
		netend_op();
		return len;
	}

	pbuf_free(pb);
	netend_op();
	return -ECONNABORTED;
}

int udprecvfrom(struct socket *s, void *buf, int len, int flags, struct sockaddr *addr, int *addrlen)
{
	struct udp_pcb *pcb = (struct udp_pcb *)s->pcb;
	if (!pcb)
	{
		return -ECONNRESET;
	}

	netbegin_op();
	udp_recv(pcb, lwip_udp_recv_callback, s);
	netend_op();

	return 0;
}

int udpclose(struct socket *s)
{
	netbegin_op();
	udp_disconnect(s->pcb);
	udp_remove(s->pcb);
	netend_op();
	return 0;
}

sockopts_t udp_opts = {
	.meta = {
		.type = SOCK_DGRAM,
		.protocol = IPPROTO_UDP,
	},
	.alloc = udpalloc,
	.connect = udpconnect,
	.bind = udpbind,
	.listen = udplisten,
	.accept = udpaccept,
	.send = udpsend,
	.recv = udprecv,
	.sendto = udpsendto,
	.recvfrom = udprecvfrom,
	.close = udpclose,
};
