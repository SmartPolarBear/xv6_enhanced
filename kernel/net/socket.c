//
// Created by bear on 10/29/2022.
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
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/raw.h"

void lwip_udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
							const ip_addr_t *addr, u16_t port);

u8_t lwip_raw_recv_callback(void *arg, struct raw_pcb *pcb, struct pbuf *p,
							const ip_addr_t *addr);

kmem_cache_t *socket_cache;

static inline void addrin_byteswap(struct sockaddr_in *addr)
{
	addr->sin_port = byteswap16(addr->sin_port);
	addr->sin_addr.addr = byteswap32(addr->sin_addr.addr);
}

void socketinit(void)
{
	socket_cache = kmem_cache_create("socket_cache", sizeof(socket_t), 0);
	if (socket_cache == NULL)
	{
		panic("socketinit: socket_cache");
	}
}

struct file *socketalloc(int domain, int type, int protocol, int *err)
{
	if (domain != AF_INET)
	{
		*err = -EINVAL;
		return NULL;
	}

	if (type != SOCK_STREAM && type != SOCK_DGRAM)
	{
		*err = -EINVAL;
		return NULL;
	}

	if (protocol != IPPROTO_TCP && protocol != IPPROTO_UDP && protocol != IPPROTO_RAW)
	{
		*err = -EINVAL;
		return NULL;
	}

	file_t *file = filealloc();
	if (file == NULL)
	{
		*err = -EINVAL;
		return NULL;
	}

	socket_t *socket = kmem_cache_alloc(socket_cache);
	if (socket == NULL)
	{
		*err = -EINVAL;
		fileclose(file);
		return NULL;
	}
	memset(socket, 0, sizeof(socket_t));

	socket->type = type;
	socket->protocol = protocol;
	socket->file = file;

	netbegin_op();

	if (protocol == IPPROTO_TCP)
	{
		socket->pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
	}
	else if (protocol == IPPROTO_UDP)
	{
		socket->pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
	}
	else if (protocol == IPPROTO_RAW)
	{
		socket->pcb = raw_new_ip_type(IPADDR_TYPE_ANY, 0/*only for IPv6*/);
	}
	else
	{
		*err = -EPROTONOSUPPORT;
	}

	tcp_arg(socket->pcb, socket);

	netend_op();

	file->type = FD_SOCKET;
	file->socket = socket;
	file->readable = TRUE;
	file->writable = TRUE;

	*err = 0;
	return file;
}

void socketclose(socket_t *skt)
{
	netbegin_op();

	if (skt->recv_buf)
	{
		pbuf_free(skt->recv_buf);
		skt->recv_buf = NULL;
	}

	if (skt->pcb)
	{
		if (skt->protocol == IPPROTO_TCP)
		{
			tcp_close(skt->pcb);
			tcp_close(skt->pcb); // ? why twice?
		}
		else if (skt->protocol == IPPROTO_UDP)
		{
			udp_disconnect(skt->pcb);
			udp_remove(skt->pcb);
		}
		else if (skt->protocol == IPPROTO_RAW)
		{
			raw_remove(skt->pcb);
		}

		skt->pcb = NULL;
	}
	netend_op();

	for (int i = 0; i < SOCKET_NBACKLOG; i++)
	{
		if (skt->backlog[i])
		{
			socketclose(skt->backlog[i]);
			skt->backlog[i] = NULL;
		}
	}

	skt->wakeup_retcode = -ECONNRESET;

	wakeup(&skt->connect_chan);
	wakeup(&skt->recv_chan);
	wakeup(&skt->accept_chan);

	kmem_cache_free(socket_cache, skt);
}

int socketconnect(socket_t *skt, struct sockaddr *addr, int addr_len)
{
	sockaddr_in_t *addr_in = (sockaddr_in_t *)addr;
	if (skt->protocol != IPPROTO_TCP && skt->protocol != IPPROTO_RAW)
	{
		return -EPROTONOSUPPORT;
	}

	if (skt->type != SOCK_STREAM)
	{
		return -EOPNOTSUPP;
	}

	addrin_byteswap(addr_in);

	netbegin_op();

	err_t err = -1;
	if (skt->protocol == IPPROTO_TCP)
	{
		err = tcp_connect(skt->pcb, (ip_addr_t *)&addr_in->sin_addr, addr_in->sin_port, NULL);
		if (err != ERR_OK)
		{
			netend_op();
			return -ECONNREFUSED;
		}
	}
	else if (skt->protocol == IPPROTO_RAW)
	{
		err = raw_connect(skt->pcb, (ip_addr_t *)&addr_in->sin_addr);
		if (err != ERR_OK)
		{
			netend_op();
			return -ECONNREFUSED;
		}
	}

	netend_op();

	if (err == ERR_OK)
	{
		acquire(&skt->lock);
		skt->wakeup_retcode = 0;
		sleep(&skt->connect_chan, &skt->lock);
		release(&skt->lock);

		if (skt->wakeup_retcode != 0)
		{
			return skt->wakeup_retcode;
		}

		return 0;
	}

	return -EINVAL;
}

int socketbind(socket_t *skt, struct sockaddr *addr, int addr_len)
{
	sockaddr_in_t *addr_in = (sockaddr_in_t *)addr;
	addrin_byteswap(addr_in);

	if (skt->protocol != IPPROTO_TCP && skt->protocol != IPPROTO_RAW)
	{
		return -EINVAL;
	}

	if (skt->type != SOCK_STREAM)
	{
		return -EOPNOTSUPP;
	}

	netbegin_op();
	err_t err = -1;
	if (skt->protocol == IPPROTO_TCP)
	{
		struct tcp_pcb *pcb = (struct tcp_pcb *)skt->pcb;
		err = tcp_bind(pcb, (ip_addr_t *)&addr_in->sin_addr, addr_in->sin_port);
	}
	else if (skt->protocol == IPPROTO_RAW)
	{
		struct raw_pcb *pcb = (struct raw_pcb *)skt->pcb;
		err = raw_bind(pcb, (ip_addr_t *)&addr_in->sin_addr);
	}
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

int socketlisten(socket_t *skt, int backlog)
{
	if (skt->protocol != IPPROTO_TCP)
	{
		return -EINVAL;
	}

	if (skt->type != SOCK_STREAM)
	{
		return -EOPNOTSUPP;
	}

	netbegin_op();

	struct tcp_pcb *pcb = (struct tcp_pcb *)skt->pcb;
	err_t err = 0;
	pcb = tcp_listen_with_backlog_and_err(pcb, backlog, &err);
	skt->pcb = pcb;

	netend_op();

	if (err == ERR_OK)
	{
		return 0;
	}

	return -EINVAL;
}

struct file *socketaccept(socket_t *skt, struct sockaddr *addr, int *addrlen, int *err)
{
	if (skt->protocol != IPPROTO_TCP)
	{
		*err = -EINVAL;
		return NULL;
	}

	if (skt->type != SOCK_STREAM)
	{
		*err = -EOPNOTSUPP;
		return NULL;
	}

	int avail = 0;

	if (FALSE) // TODO: non-blocking
	{
		for (avail = 0; avail < SOCKET_NBACKLOG; avail++)
		{
			if (skt->backlog[avail])
			{
				break;
			}
		}

		if (avail >= SOCKET_NBACKLOG)
		{
			*err = -EAGAIN;
			return NULL;
		}
	}
	else
	{
		while (!skt->backlog[avail])
		{
			acquire(&skt->lock);
			for (avail = 0; avail < SOCKET_NBACKLOG; avail++)
			{
				if (skt->backlog[avail])
				{
					release(&skt->lock);
					break;
				}
			}

			if (avail >= SOCKET_NBACKLOG)
			{
				skt->wakeup_retcode = 0;
				sleep(&skt->accept_chan, &skt->lock);
				release(&skt->lock);

				if (skt->wakeup_retcode != 0)
				{
					*err = skt->wakeup_retcode;
					return NULL;
				}
			}
		}
	}

	file_t *file = filealloc();
	if (file == NULL)
	{
		*err = -EINVAL;
		return NULL;
	}

	socket_t *socket = skt->backlog[avail];

	file->type = FD_SOCKET;
	file->socket = socket;
	file->readable = TRUE;
	file->writable = TRUE;

	struct tcp_pcb *newpcb = socket->pcb;

	struct sockaddr_in *in_addr = (struct sockaddr_in *)addr;
	addrin_byteswap(in_addr);

	in_addr->sin_addr = newpcb->remote_ip;
	in_addr->sin_port = newpcb->remote_port;
	in_addr->sin_family = AF_INET;

	*addrlen = sizeof(struct sockaddr_in);

	skt->backlog[avail] = NULL;

	*err = 0;
	return file;
}

int socketrecv(socket_t *skt, char *buf, int len, int flags)
{
	if (skt->protocol != IPPROTO_TCP && skt->protocol != IPPROTO_RAW)
	{
		return -EPROTONOSUPPORT;
	}

	if (skt->type != SOCK_STREAM)
	{
		return -EOPNOTSUPP;
	}

	if (len <= 0)
	{
		return -EINVAL;
	}

	if (FALSE)
	{
		if (skt->recv_buf == NULL)
		{
			if (skt->recv_closed)
			{
				return -ECONNRESET;
			}
			return -EAGAIN;
		}
	}
	else
	{
		if (skt->protocol == IPPROTO_RAW)
		{
			skt->raw_recv.recv_len = len;
			netbegin_op();
			struct raw_pcb *pcb = (struct raw_pcb *)skt->pcb;
			raw_recv(pcb, lwip_raw_recv_callback, skt);
			netend_op();
		}

		while (!skt->recv_buf)
		{
			if (skt->recv_closed)
			{
				return -ECONNRESET;
			}
			acquire(&skt->lock);
			skt->wakeup_retcode = 0;
			sleep(&skt->recv_chan, &skt->lock);
			release(&skt->lock);

			if (skt->wakeup_retcode != 0)
			{
				return skt->wakeup_retcode;
			}
		}
	}
	netbegin_op();

	len = pbuf_copy_partial(skt->recv_buf, buf, len, skt->recv_offset);
	skt->recv_offset += len;
	KDEBUG_ASSERT(skt->recv_offset <= skt->recv_buf->tot_len);

	if (skt->recv_offset == skt->recv_buf->tot_len)
	{
		pbuf_free(skt->recv_buf);
		skt->recv_buf = NULL;
		skt->recv_offset = 0;
	}

	netend_op();

	return len;
}

int socketsend(socket_t *skt, char *buf, int len, int flags)
{
	if (skt->protocol != IPPROTO_TCP && skt->protocol != IPPROTO_RAW)
	{
		return -EPROTONOSUPPORT;
	}

	if (skt->type != SOCK_STREAM)
	{
		return -EOPNOTSUPP;
	}

	netbegin_op();

	if (skt->protocol == IPPROTO_TCP)
	{
		struct tcp_pcb *pcb = (struct tcp_pcb *)skt->pcb;
		if (!pcb)
		{
			return -ECONNRESET;
		}

		if (!pcb->snd_buf)
		{
			return -EAGAIN;
		}

		len = MIN(len, pcb->snd_buf);
		err_t err = tcp_write(pcb, buf, len, TCP_WRITE_FLAG_COPY);

		if (err == ERR_MEM)
		{
			return -ENOMEM;
		}
		else if (err != ERR_OK)
		{
			return -ECONNABORTED;
		}

		err = tcp_output(pcb);
		if (err == ERR_OK)
		{
			return len;
		}

		if (err == ERR_MEM)
		{
			return -EAGAIN;
		}
	}
	else if (skt->protocol == IPPROTO_RAW)
	{
		struct raw_pcb *pcb = (struct raw_pcb *)skt->pcb;
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
	}
	netend_op();

	return -ECONNABORTED;
}

int socketioctl(socket_t *skt, int req, void *arg)
{
	return 0;
}

int socketgetsockopt(socket_t *skt, int level, int optname, void *optval, int *optlen)
{
	switch (skt->type)
	{
	case IPPROTO_TCP:
		switch (optname)
		{
		case SO_TYPE:
			*(int *)optval = SOCK_STREAM;
			*optlen = sizeof(int);
			return 0;
		}
		break;
	case IPPROTO_UDP:
		switch (optname)
		{
		case SO_TYPE:
			*(int *)optval = SOCK_STREAM;
			*optlen = sizeof(int);
			return 0;
		}
		break;
	}
	return -EINVAL;
}

int socketsendto(socket_t *skt, char *buf, int len, int flags, struct sockaddr *addr, int addrlen)
{
	if (skt->protocol != IPPROTO_UDP && skt->protocol != IPPROTO_RAW)
	{
		if (addr)
		{
			return -EISCONN;
		}

		// as stated in the document, it will have the same effect as send()
		return socketsend(skt, buf, len, flags);
	}

	if (addr == NULL)
	{
		return -EDESTADDRREQ;
	}

	if (addrlen < sizeof(struct sockaddr_in))
	{
		return -EINVAL;
	}

	struct sockaddr_in *in_addr = (struct sockaddr_in *)addr;
	addrin_byteswap(in_addr);

	if (in_addr->sin_family != AF_INET)
	{
		return -EAFNOSUPPORT;
	}

	netbegin_op();

	if (skt->protocol == IPPROTO_UDP)
	{
		struct udp_pcb *pcb = (struct udp_pcb *)skt->pcb;
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
		if (err != ERR_OK)
		{
			pbuf_free(pb);
			netend_op();
			return -ECONNABORTED;
		}

		pbuf_free(pb);
	}
	else if (skt->protocol == IPPROTO_RAW)
	{
		struct raw_pcb *pcb = (struct raw_pcb *)skt->pcb;
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
	}

	netend_op();

	return -EINVAL;
}

int socketrecvfrom(socket_t *skt, char *buf, int len, int flags, struct sockaddr *addr, int *addrlen)
{
	if (skt->protocol != IPPROTO_UDP)
	{
		if (addr)
		{
			return -EISCONN;
		}

		// as stated in the document, it will have the same effect as recv()
		return socketrecv(skt, buf, len, flags);
	}

	if (skt->type != SOCK_DGRAM)
	{
		return -EOPNOTSUPP;
	}

	if (addr == NULL)
	{
		return -EDESTADDRREQ;
	}

	if (addrlen && *addrlen < sizeof(struct sockaddr_in))
	{
		return -EINVAL;
	}

	struct sockaddr_in *in_addr = (struct sockaddr_in *)addr;
	addrin_byteswap(in_addr);

	if (in_addr->sin_family != AF_INET)
	{
		return -EAFNOSUPPORT;
	}

	skt->udp_recvfrom.recv_addr = addr;
	skt->udp_recvfrom.recv_addrlen = addrlen;
	skt->udp_recvfrom.recv_len = len;

	struct udp_pcb *pcb = (struct udp_pcb *)skt->pcb;
	if (!pcb)
	{
		return -ECONNRESET;
	}

	netbegin_op();
	udp_recv(pcb, lwip_udp_recv_callback, skt);
	netend_op();

	if (skt->recv_buf == NULL)
	{
//		if (flags & MSG_DONTWAIT)
//		{
//			return -EAGAIN;
//		}

		acquire(&skt->lock);
		skt->wakeup_retcode = 0;
		sleep(&skt->recv_chan, &skt->lock);
		release(&skt->lock);

		if (skt->wakeup_retcode != 0)
		{
			return skt->wakeup_retcode;
		}
	}

	netbegin_op();

	len = pbuf_copy_partial(skt->recv_buf, buf, len, skt->recv_offset);
	skt->recv_offset += len;
	KDEBUG_ASSERT(skt->recv_offset <= skt->recv_buf->tot_len);

	if (skt->recv_offset == skt->recv_buf->tot_len)
	{
		pbuf_free(skt->recv_buf);
		skt->recv_buf = NULL;
		skt->recv_offset = 0;
		if (skt->protocol == IPPROTO_UDP)
		{
			memset(&skt->udp_recvfrom, 0, sizeof(skt->udp_recvfrom));
		}
		else if (skt->protocol == IPPROTO_RAW)
		{
			memset(&skt->raw_recv, 0, sizeof(skt->udp_recvfrom));
		}
	}

	netend_op();

	return len;
}

err_t lwip_tcp_event(void *arg, struct tcp_pcb *pcb, enum lwip_event event, struct pbuf *p,
					 u16_t size, err_t err)
{
	socket_t *socket = (socket_t *)arg;

	switch (event)
	{
	case LWIP_EVENT_ACCEPT:
		if (err == ERR_OK)
		{
			int free = 0;
			for (free = 0; free < SOCKET_NBACKLOG; ++free)
			{
				if (!socket->backlog[free])
				{
					break;
				}
			}
			// queue full
			if (free >= SOCKET_NBACKLOG)
			{
				return ERR_MEM;
			}
			socket_t *newsocket = kmem_cache_alloc(socket_cache);
			memset(newsocket, 0, sizeof(socket_t));

			// the passed in pcb is for the new socket
			newsocket->pcb = pcb;

			netbegin_op();
			tcp_arg(pcb, newsocket);
			netend_op();

			newsocket->protocol = socket->protocol;
			newsocket->type = socket->type;

			socket->backlog[free] = newsocket;

			socket->wakeup_retcode = 0;
			wakeup(&socket->accept_chan);
		}
		return ERR_OK;
	case LWIP_EVENT_SENT:
		/* ignore */
		return ERR_OK;
	case LWIP_EVENT_RECV:
		/* closed or error */
		if (!p || err != ERR_OK)
		{
			if (p)
			{
				netbegin_op();
				pbuf_free(p);
				netend_op();
			}
			socket->recv_closed = TRUE;
			return ERR_OK;
		}
		// buffer hasn't been received
		if (socket->recv_buf)
		{
			return ERR_MEM;
		}
		/* ack the packet */
		netbegin_op();
		tcp_recved(socket->pcb, p->tot_len);
		netend_op();

		socket->recv_buf = p;
		socket->recv_offset = 0;

		socket->wakeup_retcode = 0;
		wakeup(&socket->recv_chan);
		return ERR_OK;
	case LWIP_EVENT_CONNECTED:
		if (err != ERR_OK)
		{
			socket->wakeup_retcode = -ECONNRESET;
		}
		else
		{
			socket->wakeup_retcode = 0;
		}

		wakeup(&socket->connect_chan);
		return ERR_OK;
	case LWIP_EVENT_POLL:
		/* ignore */
		return ERR_OK;
	case LWIP_EVENT_ERR:
		/* pcb is already deallocated */
		socket->pcb = NULL;
		// do nothing. de-allocation will be done by exit() or manually by the user
		return ERR_ABRT;
	default:
		break;
	}

	KDEBUG_UNREACHABLE;
}

void lwip_udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
							const ip_addr_t *addr, u16_t port)
{
	socket_t *socket = (socket_t *)arg;
	struct sockaddr_in *sin = (struct sockaddr_in *)&socket->udp_recvfrom.recv_addr;
	if (sin->sin_addr.addr != addr->addr || sin->sin_port != port)
	{
		return;
	}

	if (!p || p->tot_len > socket->udp_recvfrom.recv_len)
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

u8_t lwip_raw_recv_callback(void *arg, struct raw_pcb *pcb, struct pbuf *p,
							const ip_addr_t *addr)
{
	socket_t *socket = (socket_t *)arg;
	struct sockaddr_in *sin = (struct sockaddr_in *)&socket->udp_recvfrom.recv_addr;
	if (sin->sin_addr.addr != addr->addr)
	{
		return 0;
	}

	if (!p || p->tot_len > socket->raw_recv.recv_len)
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
