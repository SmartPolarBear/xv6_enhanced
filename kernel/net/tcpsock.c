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

extern kmem_cache_t *socket_cache;

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
			newsocket->opts = socket->opts;

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

int tcpalloc(struct socket *socket)
{
	netbegin_op();
	socket->pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
	if (socket->pcb == NULL)
	{
		netend_op();
		return -ENOMEM;
	}
	tcp_arg(socket->pcb, socket);
	netend_op();
	return 0;
}

int tcpconnect(struct socket *s, struct sockaddr *addr, int addrlen)
{
	sockaddr_in_t *addr_in = (sockaddr_in_t *)addr;
	addrin_byteswap(addr_in);

	netbegin_op();
	err_t err = tcp_connect(s->pcb, (ip_addr_t *)&addr_in->sin_addr, addr_in->sin_port, NULL);
	if (err != ERR_OK)
	{
		netend_op();
		return -ECONNREFUSED;
	}
	netend_op();
	return 0;
}

int tcpbind(struct socket *s, struct sockaddr *addr, int addrlen)
{
	sockaddr_in_t *addr_in = (sockaddr_in_t *)addr;
	addrin_byteswap(addr_in);

	netbegin_op();
	struct tcp_pcb *pcb = (struct tcp_pcb *)s->pcb;
	err_t err = tcp_bind(pcb, (ip_addr_t *)&addr_in->sin_addr, addr_in->sin_port);
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

int tcplisten(struct socket *s, int backlog)
{
	netbegin_op();
	struct tcp_pcb *pcb = (struct tcp_pcb *)s->pcb;
	err_t err = 0;
	pcb = tcp_listen_with_backlog_and_err(pcb, backlog, &err);
	s->pcb = pcb;
	netend_op();

	if (err == ERR_OK)
	{
		return 0;
	}

	return -EINVAL;
}

int tcpaccept(struct socket *s, struct socket *newsock, struct sockaddr *addr, int *addrlen)
{
	netbegin_op();
	struct tcp_pcb *newpcb = newsock->pcb;

	struct sockaddr_in *in_addr = (struct sockaddr_in *)addr;
	in_addr->sin_addr = newpcb->remote_ip;
	in_addr->sin_port = newpcb->remote_port;
	in_addr->sin_family = AF_INET;
	addrin_byteswap(in_addr);

	*addrlen = sizeof(struct sockaddr_in);

	netend_op();

	return 0;
}

int tcpsend(struct socket *s, void *buf, int len, int flags)
{
	struct tcp_pcb *pcb = (struct tcp_pcb *)s->pcb;
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

	if (err == ERR_MEM)
	{
		netend_op();
		return -ENOMEM;
	}
	else if (err != ERR_OK)
	{
		netend_op();
		return -ECONNABORTED;
	}

	err = tcp_output(pcb);
	if (err == ERR_OK)
	{
		netend_op();
		return len; // succeeded
	}

	if (err == ERR_MEM)
	{
		netend_op();
		return -EAGAIN;
	}


	// error condition
	netend_op();
	return -ECONNABORTED;

}

int tcprecv(struct socket *s, void *buf, int len, int flags)
{
	return 0; // do nothing. event loop will do everything
}

int tcpsendto(struct socket *s, void *buf, int len, int flags, struct sockaddr *addr, int addrlen)
{
	return -EPROTONOSUPPORT;
}

int tcprecvfrom(struct socket *s, void *buf, int len, int flags, struct sockaddr *addr, int *addrlen)
{
	return -EPROTONOSUPPORT;
}

int tcpclose(struct socket *s)
{
	netbegin_op();
	tcp_close(s->pcb);
	tcp_close(s->pcb); // ? why twice?
	netend_op();
	return 0;
}

sockopts_t tcp_opts = {
	.meta = {
		.type = SOCK_STREAM,
		.protocol = IPPROTO_TCP,
	},
	.alloc = tcpalloc,
	.connect = tcpconnect,
	.bind = tcpbind,
	.listen = tcplisten,
	.accept = tcpaccept,
	.send = tcpsend,
	.recv = tcprecv,
	.sendto = tcpsendto,
	.recvfrom = tcprecvfrom,
	.close = tcpclose,
};