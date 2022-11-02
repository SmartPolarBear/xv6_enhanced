//
// Created by bear on 10/31/2022.
//

#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"
#include "socket.h"

int
sys_socket(void)
{
	int fd, domain, type, protocol;
	struct file *f;

	if (argint(0, &domain) < 0 || argint(1, &type) < 0 || argint(2, &protocol) < 0)
	{
		return -1;
	}

	int err = 0;
	f = socketalloc(domain, type, protocol, &err);
	if (err != 0 || (fd = fdalloc(f)) < 0)
	{
		if (f)
		{
			fileclose(f);
		}
		return err;
	}
	return fd;
}

int
sys_connect(void)
{
	struct file *f;
	int addrlen;
	struct sockaddr *addr;

	if (argfd(0, NULL, &f) < 0 || argint(2, &addrlen) < 0 || argptr(1, (void *)&addr, addrlen) < 0)
	{
		return -1;
	}
	if (f->type != FD_SOCKET)
	{
		return -1;
	}
	return socketconnect(f->socket, addr, addrlen);
}

int
sys_bind(void)
{
	struct file *f;
	int addrlen;
	struct sockaddr *addr;

	if (argfd(0, NULL, &f) < 0 || argint(2, &addrlen) < 0 || argptr(1, (void *)&addr, addrlen) < 0)
	{
		return -1;
	}
	if (f->type != FD_SOCKET)
	{
		return -1;
	}
	return socketbind(f->socket, addr, addrlen);
}

int
sys_listen(void)
{
	struct file *f;
	int backlog;

	if (argfd(0, NULL, &f) < 0 || argint(1, &backlog) < 0)
	{
		return -1;
	}
	if (f->type != FD_SOCKET)
	{
		return -1;
	}
	return socketlisten(f->socket, backlog);
}

int
sys_accept(void)
{
	struct file *f, *af;
	int *addrlen, afd;
	struct sockaddr *addr = NULL;

	if (argfd(0, NULL, &f) < 0 || argptr(2, (void *)&addrlen, sizeof(*addrlen)) < 0)
	{
		return -1;
	}
	if (addrlen && argptr(1, (void *)&addr, *addrlen) < 0)
	{
		return -1;
	}
	if (f->type != FD_SOCKET)
	{
		return -1;
	}

	int err = 0;
	af = socketaccept(f->socket, addr, addrlen, &err);
	if (err < 0)
	{
		return err;
	}

	afd = fdalloc(af);
	return afd;
}

int
sys_recv(void)
{
	struct file *f;
	int n;
	char *p;
	int flags;

	if (argfd(0, NULL, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0 || argint(3, &flags) < 0)
	{
		return -1;
	}

	if (f->type != FD_SOCKET)
	{
		return -1;
	}
	return socketrecv(f->socket, p, n, flags);
}

int
sys_send(void)
{
	struct file *f;
	int n;
	char *p;
	int flags;

	if (argfd(0, NULL, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0 || argint(3, &flags) < 0)
	{
		return -1;
	}

	if (f->type != FD_SOCKET)
	{
		return -1;
	}
	return socketsend(f->socket, p, n, flags);
}

int
sys_recvfrom(void)
{
//	struct file *f;
//	int n;
//	char *p;
//	int *addrlen;
//	struct sockaddr *addr = NULL;
//
//	if (argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0
//		|| argptr(4, (void *)&addrlen, sizeof(*addrlen)) < 0)
//	{
//		return -1;
//	}
//	if (addrlen && argptr(3, (void *)&addr, *addrlen) < 0)
//	{
//		return -1;
//	}
//	if (f->type != FD_SOCKET)
//	{
//		return -1;
//	}
//	return socketrecvfrom(f->socket, p, n, addr, addrlen);
	return -1;
}

int
sys_sendto(void)
{
//	struct file *f;
//	int n;
//	char *p;
//	int addrlen;
//	struct sockaddr *addr;
//
//	if (argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0 || argint(4, &addrlen) < 0
//		|| argptr(3, (void *)&addr, addrlen) < 0)
//	{
//		return -1;
//	}
//	if (f->type != FD_SOCKET)
//	{
//		return -1;
//	}
//	return socketsendto(f->socket, p, n, addr, addrlen);
	return -1;
}
