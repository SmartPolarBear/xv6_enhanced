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

#include "lwip/raw.h"

static inline u8_t lwip_raw_recv_callback(void *arg, struct raw_pcb *pcb, struct pbuf *p,
										  const ip_addr_t *addr)
{
	socket_t *socket = (socket_t *)arg;
	socket->recvfrom_params.recv_addr.sin_addr.addr = addr->addr;
	socket->recvfrom_params.recv_addr.sin_port = 0;
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
		return 0;
	}
	// buffer hasn't been received
	if (socket->recv_buf)
	{
		return 0;
	}

	socket->recv_buf = p;
	socket->recv_offset = 0;

	socket->wakeup_retcode = 0;
	wakeup(&socket->recv_chan);

	return p->tot_len;
}

int rawalloc(struct socket *socket)
{
	uint8 proto = 0;
	switch (socket->protocol)
	{
	case IPPROTO_TCP:
		proto = IP_PROTO_TCP;
		break;
	case IPPROTO_UDP:
		proto = IP_PROTO_UDP;
		break;
	case IPPROTO_ICMP:
		proto = IP_PROTO_ICMP;
		break;
	default:
		return -EPROTONOSUPPORT;
	}

	netbegin_op();
	socket->pcb = raw_new_ip_type(IPADDR_TYPE_ANY, proto);
	if (socket->pcb == NULL)
	{
		netend_op();
		return -ENOMEM;
	}
	netend_op();
	return 0;
}

int rawconnect(struct socket *s, struct sockaddr *addr, int addrlen)
{
	sockaddr_in_t *addr_in = (sockaddr_in_t *)addr;
	addrin_byteswap(addr_in);

	netbegin_op();
	err_t err = raw_connect(s->pcb, (ip_addr_t *)&addr_in->sin_addr);
	if (err != ERR_OK)
	{
		netend_op();
		return -ECONNREFUSED;
	}
	netend_op();
	return 0;
}

int rawbind(struct socket *s, struct sockaddr *addr, int addrlen)
{
	sockaddr_in_t *addr_in = (sockaddr_in_t *)addr;
	addrin_byteswap(addr_in);

	netbegin_op();
	struct raw_pcb *pcb = (struct raw_pcb *)s->pcb;
	err_t err = raw_bind(pcb, (ip_addr_t *)&addr_in->sin_addr);
	netend_op();

	if (err == ERR_OK)
	{
		return 0;
	}
	else if (err == ERR_USE)
	{
		return -EADDRINUSE;
	}

	return 0;
}

int rawlisten(struct socket *s, int backlog)
{
	return -EPROTONOSUPPORT;
}

int rawaccept(struct socket *s, struct socket *newsock, struct sockaddr *addr, int *addrlen)
{
	return -EPROTONOSUPPORT;
}

int rawsend(struct socket *s, void *buf, int len, int flags)
{
	netbegin_op();

	struct raw_pcb *pcb = (struct raw_pcb *)s->pcb;
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

	err = raw_send(pcb, pb);
	if (err != ERR_OK)
	{
		pbuf_free(pb);
		netend_op();
		return -ECONNABORTED;
	}

	pbuf_free(pb);
	netend_op();
	return len;
}

int rawrecv(struct socket *s, void *buf, int len, int flags)
{
	s->recvfrom_params.recv_len = len;
	netbegin_op();
	struct raw_pcb *pcb = (struct raw_pcb *)s->pcb;
	raw_recv(pcb, lwip_raw_recv_callback, s);
	netend_op();
	return 0;
}

int rawsendto(struct socket *s, void *buf, int len, int flags, struct sockaddr *addr, int addrlen)
{
	struct sockaddr_in *in_addr = (struct sockaddr_in *)addr;
	addrin_byteswap(in_addr);

	if (in_addr->sin_family != AF_INET)
	{
		return -EAFNOSUPPORT;
	}

	netbegin_op();
	struct raw_pcb *pcb = (struct raw_pcb *)s->pcb;
	struct pbuf *pb = pbuf_alloc(PBUF_IP, len, PBUF_RAM);
	err_t err = pbuf_take(pb, buf, len);
	if (err != ERR_OK)
	{
		pbuf_free(pb);
		netend_op();
		return -ENOMEM;
	}

	err = raw_sendto(pcb, pb, &in_addr->sin_addr);
	if (err != ERR_OK)
	{
		pbuf_free(pb);
		netend_op();
		return -ECONNABORTED;
	}

	pbuf_free(pb);
	netend_op();

	return len;
}

int rawrecvfrom(struct socket *s, void *buf, int len, int flags, struct sockaddr *addr, int *addrlen)
{
	struct raw_pcb *pcb = (struct raw_pcb *)s->pcb;
	if (!pcb)
	{
		return -ECONNRESET;
	}

	netbegin_op();
	raw_recv(pcb, lwip_raw_recv_callback, s);
	netend_op();
	return 0;
}

int rawclose(struct socket *socket)
{
	netbegin_op();
	raw_remove(socket->pcb);
	netend_op();
	return 0;
}

int rawgetsockopt(struct socket *s, int level, int optname, void *optval, int *optlen)
{
	switch (level)
	{
	case IPPROTO_IP:
		switch (optname)
		{
		case IP_TTL:
			*(int *)optval = ((struct raw_pcb *)s->pcb)->ttl;
			*optlen = sizeof(int);
			return 0;
		}
		return -ENOPROTOOPT;

	case IPPROTO_RAW:
		switch (optname)
		{
		case SO_TYPE:
			*(int *)optval = s->type;
			*optlen = sizeof(int);
			return 0;
		}
	}
	return -ENOPROTOOPT;
}

int rawsetsockopt(struct socket *s, int level, int optname, void *optval, int optlen)
{
	switch (level)
	{
	case IPPROTO_IP:
		switch (optname)
		{
		case IP_TTL:
			((struct raw_pcb *)s->pcb)->ttl = *(int *)optval;
			return 0;
		}
		return -ENOPROTOOPT;
	case IPPROTO_RAW:
		switch (optname)
		{
		case SO_TYPE:
			s->type = *(int *)optval;
			return 0;
		}
	}
	return -ENOPROTOOPT;
}

sockopts_t raw_opts = {
	.meta={
		.type=SOCK_RAW,
		.protocol=-1, // match any
	},
	.alloc=rawalloc,
	.connect=rawconnect,
	.bind=rawbind,
	.listen=rawlisten,
	.accept=rawaccept,
	.send=rawsend,
	.recv=rawrecv,
	.sendto=rawsendto,
	.recvfrom=rawrecvfrom,
	.close=rawclose,
	.getsockopt=rawgetsockopt,
	.setsockopt=rawsetsockopt,
};

