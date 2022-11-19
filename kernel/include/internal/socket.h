//
// Created by bear on 11/19/2022.
//

#pragma once

typedef struct sockopts
{
	struct
	{
		int type, protocol;
	} meta;

	int (*alloc)(struct socket *s);
	int (*connect)(struct socket *s, struct sockaddr *addr, int addrlen);
	int (*bind)(struct socket *s, struct sockaddr *addr, int addrlen);
	int (*listen)(struct socket *s, int backlog);
	int (*accept)(struct socket *s, struct socket *newsock, struct sockaddr *addr, int *addrlen);
	int (*send)(struct socket *s, void *buf, int len, int flags);
	int (*recv)(struct socket *s, void *buf, int len, int flags);
	int (*sendto)(struct socket *s, void *buf, int len, int flags, struct sockaddr *addr, int addrlen);
	int (*recvfrom)(struct socket *s, void *buf, int len, int flags, struct sockaddr *addr, int *addrlen);
	int (*close)(struct socket *s);
} sockopts_t;

static inline void addrin_byteswap(struct sockaddr_in *addr)
{
	addr->sin_port = byteswap16(addr->sin_port);
	addr->sin_addr.addr = byteswap32(addr->sin_addr.addr);
}

