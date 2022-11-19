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
#include "internal/socket.h"

#include "lwip/pbuf.h"

kmem_cache_t *socket_cache;

extern sockopts_t tcp_opts;
extern sockopts_t udp_opts;
extern sockopts_t raw_opts;

sockopts_t *sockopts[] = {
	&tcp_opts,
	&udp_opts,
	&raw_opts,
};

static inline sockopts_t *find_sockopts(int type, int proto)
{
	for (int i = 0; i < NELEM(sockopts); i++)
	{
		if (sockopts[i]->meta.type == type && sockopts[i]->meta.protocol == proto)
		{
			return sockopts[i];
		}
		else if (sockopts[i]->meta.type == type && sockopts[i]->meta.protocol == -1) // match any protocol
		{
			return sockopts[i];
		}
	}
	return NULL;
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

	if (type != SOCK_STREAM && type != SOCK_DGRAM && type != SOCK_RAW)
	{
		*err = -EINVAL;
		return NULL;
	}

	if (protocol != IPPROTO_TCP && protocol != IPPROTO_UDP && protocol != IPPROTO_RAW
		&& protocol != IPPROTO_ICMP)
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
	socket->opts = find_sockopts(type, protocol);
	if (socket->opts == NULL)
	{
		*err = -ESOCKTNOSUPPORT;
		goto free_sock;
	}

	int ret = socket->opts->alloc(socket);
	if (ret < 0)
	{
		*err = ret;
		goto free_sock;
	}

	file->type = FD_SOCKET;
	file->socket = socket;
	file->readable = TRUE;
	file->writable = TRUE;

	*err = 0;
	return file;

free_sock:
	kmem_cache_free(socket_cache, socket);
	netend_op();
	return NULL;
}

void socketclose(socket_t *skt)
{
	netbegin_op();
	if (skt->recv_buf)
	{
		pbuf_free(skt->recv_buf);
		skt->recv_buf = NULL;
	}
	netend_op();

	if (skt->pcb)
	{
		KDEBUG_ASSERT(!skt->opts->close(skt));
		skt->pcb = NULL;
	}

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
	if (skt->type != SOCK_STREAM && skt->type != SOCK_RAW)
	{
		return -EOPNOTSUPP;
	}

	int ret = skt->opts->connect(skt, addr, addr_len);

	if (!ret)
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

	return ret;
}

int socketbind(socket_t *skt, struct sockaddr *addr, int addr_len)
{
	if (skt->type != SOCK_STREAM && skt->type != SOCK_DGRAM)
	{
		return -EOPNOTSUPP;
	}

	return skt->opts->bind(skt, addr, addr_len);
}

int socketlisten(socket_t *skt, int backlog)
{
	if (skt->type != SOCK_STREAM)
	{
		return -EOPNOTSUPP;
	}

	return skt->opts->listen(skt, backlog);
}

struct file *socketaccept(socket_t *skt, struct sockaddr *addr, int *addrlen, int *err)
{
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

	*err = skt->opts->accept(skt, socket, addr, addrlen);

	skt->backlog[avail] = NULL;

	return file;
}

int socketrecv(socket_t *skt, char *buf, int len, int flags)
{
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
		int err = skt->opts->recv(skt, buf, len, flags);
		if (err)
		{
			return err;
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
	if (skt->type != SOCK_STREAM)
	{
		return -EOPNOTSUPP;
	}

	return skt->opts->send(skt, buf, len, flags);
}

int socketsendto(socket_t *skt, char *buf, int len, int flags, struct sockaddr *addr, int addrlen)
{
	if (skt->type == SOCK_STREAM)
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

	if (addrlen != sizeof(struct sockaddr_in))
	{
		return -EINVAL;
	}

	return skt->opts->sendto(skt, buf, len, flags, addr, addrlen);
}

int socketrecvfrom(socket_t *skt, char *buf, int len, int flags, struct sockaddr *addr, int *addrlen)
{
	if (skt->type == SOCK_STREAM)
	{
		if (addr)
		{
			return -EISCONN;
		}

		// as stated in the document, it will have the same effect as recv()
		return socketrecv(skt, buf, len, flags);
	}

	skt->recvfrom_params.recv_len = len;

	int err = skt->opts->recvfrom(skt, buf, len, flags, addr, addrlen);
	if (err)
	{
		return err;
	}

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

		memmove(addr, &skt->recvfrom_params.recv_addr, sizeof(struct sockaddr_in));
		*addrlen = sizeof(struct sockaddr_in);
		addrin_byteswap((struct sockaddr_in *)addr);

		memset(&skt->recvfrom_params, 0, sizeof(skt->recvfrom_params));
	}

	netend_op();

	return len;
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

int socksetsockopt(socket_t *skt, int level, int optname, void *optval, int optlen)
{
	switch (skt->type)
	{
	case IPPROTO_TCP:
		switch (optname)
		{
		case SO_TYPE:
			return -EINVAL;
		}
		break;
	case IPPROTO_UDP:
		switch (optname)
		{
		case SO_TYPE:
			return -EINVAL;
		}
		break;
	}
	return -EINVAL;
}