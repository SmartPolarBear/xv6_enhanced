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

int tcpalloc(struct socket *s)
{
	netbegin_op();

	netend_op();
	return 0;
}

int tcpconnect(struct socket *s, struct sockaddr *addr, int addrlen)
{
	netbegin_op();

	netend_op();
	return 0;
}

int tcpbind(struct socket *s, struct sockaddr *addr, int addrlen)
{
	netbegin_op();

	netend_op();
	return 0;
}

int tcplisten(struct socket *s, int backlog)
{
	netbegin_op();

	netend_op();
	return 0;
}

int tcpaccept(struct socket *s, struct socket *newsock, struct sockaddr *addr, int *addrlen)
{
	netbegin_op();

	netend_op();
	return 0;
}

int tcpsend(struct socket *s, void *buf, int len, int flags)
{
	netbegin_op();

	netend_op();
	return 0;
}

int tcprecv(struct socket *s, void *buf, int len, int flags)
{
	netbegin_op();

	netend_op();
	return 0;
}

int tcpsendto(struct socket *s, void *buf, int len, int flags, struct sockaddr *addr, int addrlen)
{
	netbegin_op();

	netend_op();
	return 0;
}

int tcprecvfrom(struct socket *s, void *buf, int len, int flags, struct sockaddr *addr, int *addrlen)
{
	netbegin_op();

	netend_op();
	return 0;
}

int tcpclose(struct socket *s)
{
	netbegin_op();

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