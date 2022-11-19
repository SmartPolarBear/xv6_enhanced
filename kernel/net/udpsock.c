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

int udpalloc(struct socket *s)
{
	netbegin_op();

	netend_op();
	return 0;
}

int udpconnect(struct socket *s, struct sockaddr *addr, int addrlen)
{
	netbegin_op();

	netend_op();
	return 0;
}

int udpbind(struct socket *s, struct sockaddr *addr, int addrlen)
{
	netbegin_op();

	netend_op();
	return 0;
}

int udplisten(struct socket *s, int backlog)
{
	netbegin_op();

	netend_op();
	return 0;
}

int udpaccept(struct socket *s, struct socket *newsock, struct sockaddr *addr, int *addrlen)
{
	netbegin_op();

	netend_op();
	return 0;
}

int udpsend(struct socket *s, void *buf, int len, int flags)
{
	netbegin_op();

	netend_op();
	return 0;
}

int udprecv(struct socket *s, void *buf, int len, int flags)
{
	netbegin_op();

	netend_op();
	return 0;
}

int udpsendto(struct socket *s, void *buf, int len, int flags, struct sockaddr *addr, int addrlen)
{
	netbegin_op();

	netend_op();
	return 0;
}

int udprecvfrom(struct socket *s, void *buf, int len, int flags, struct sockaddr *addr, int *addrlen)
{
	netbegin_op();

	netend_op();
	return 0;
}

int udpclose(struct socket *s)
{
	netbegin_op();

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
