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

	if (protocol != IPPROTO_TCP && protocol != IPPROTO_UDP)
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
		if (skt->type == SOCK_STREAM)
		{
			tcp_close(skt->pcb);
		}
		else
		{
			udp_remove(skt->pcb);
		}
		tcp_close(skt->pcb); // ? why twice?
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
	if (skt->protocol != IPPROTO_TCP)
	{
		return -EINVAL;
	}

	addrin_byteswap(addr_in);

	netbegin_op();
	struct tcp_pcb *pcb = (struct tcp_pcb *)skt->pcb;
	err_t e = tcp_connect(pcb, &addr_in->sin_addr, addr_in->sin_port, NULL);
	netend_op();

	if (e == ERR_OK)
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

	if (skt->protocol != IPPROTO_TCP)
	{
		return -EINVAL;
	}

	netbegin_op();
	struct tcp_pcb *pcb = (struct tcp_pcb *)skt->pcb;
	err_t err = tcp_bind(pcb, &addr_in->sin_addr, addr_in->sin_port);
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
	if (skt->protocol != IPPROTO_TCP)
	{
		return -EINVAL;
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
	if (skt->protocol != IPPROTO_TCP)
	{
		return -EINVAL;
	}

	struct tcp_pcb *pcb = (struct tcp_pcb *)skt->pcb;
	if (!pcb)
	{
		return -ECONNRESET;
	}

	if (!pcb->snd_buf)
	{
		return -EAGAIN;
	}

	netbegin_op();
	len = MIN(len, pcb->snd_buf);

	err_t err = tcp_write(pcb, buf, len, TCP_WRITE_FLAG_COPY);
	netend_op();

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

	return -ECONNABORTED;
}

int socketioctl(socket_t *skt, int req, void *arg)
{
	return 0;
}

int socketgetsockopt(socket_t *skt, int level, int optname, void *optval, int *optlen)
{
	return 0;
}

int socketsendto(socket_t *skt, char *buf, int len, int flags, struct sockaddr *addr, int addrlen)
{
	return -EINVAL;
}

int socketrecvfrom(socket_t *skt, char *buf, int len, int flags, struct sockaddr *addr, int *addrlen)
{
	return -EINVAL;
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