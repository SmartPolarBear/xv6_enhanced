//
// Created by bear on 10/31/2022.
//

#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "errno.h"
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
	if (err != 0 || !f || (fd = fdalloc(f)) < 0)
	{
		if (f)
		{
			fileclose(f);
		}
		seterror(err);
		return -1;
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
		seterror(-ENOTSOCK);
		return -1;
	}
	int err = socketconnect(f->socket, addr, addrlen);
	if (err < 0)
	{
		seterror(err);
		return -1;
	}
	return 0;
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
		seterror(-ENOTSOCK);
		return -1;
	}
	int err = socketbind(f->socket, addr, addrlen);
	if (err < 0)
	{
		seterror(err);
		return -1;
	}
	return 0;
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
		seterror(-ENOTSOCK);
		return -1;
	}
	int err = socketlisten(f->socket, backlog);
	if (err < 0)
	{
		seterror(err);
		return -1;
	}
	return 0;
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
		seterror(-ENOTSOCK);
		return -1;
	}

	int err = 0;
	af = socketaccept(f->socket, addr, addrlen, &err);
	if (err < 0)
	{
		seterror(err);
		return -1;
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
		seterror(-ENOTSOCK);
		return -1;
	}
	int err = socketrecv(f->socket, p, n, flags);
	if (err < 0)
	{
		seterror(err);
		return -1;
	}
	return err; // length
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
		seterror(-ENOTSOCK);
		return -1;
	}

	int err = socketsend(f->socket, p, n, flags);
	if (err < 0)
	{
		seterror(err);
		return -1;
	}
	return err; // length
}

int
sys_recvfrom(void)
{
	struct file *f;
	int n;
	char *p;
	int flags;
	int *addrlen;
	struct sockaddr *addr = NULL;

	if (argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0 || argint(3, &flags) < 0
		|| argptr(5, (void *)&addrlen, sizeof(*addrlen)) < 0)
	{
		return -1;
	}

	if (addrlen && argptr(4, (void *)&addr, *addrlen) < 0)
	{
		return -1;
	}

	if (f->type != FD_SOCKET)
	{
		seterror(-ENOTSOCK);
		return -1;
	}

	int err = socketrecvfrom(f->socket, p, n, flags, addr, addrlen);

	if (err < 0)
	{
		seterror(err);
		return -1;
	}
	return err; // length
}

int
sys_sendto(void)
{
	struct file *f;
	int n;
	int flags;
	char *p;
	int addrlen;
	struct sockaddr *addr;

	if (argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0 || argint(3, &flags) < 0
		|| argint(5, &addrlen) < 0
		|| argptr(4, (void *)&addr, addrlen) < 0)
	{
		return -1;
	}

	if (f->type != FD_SOCKET)
	{
		seterror(-ENOTSOCK);
		return -1;
	}
	int err = socketsendto(f->socket, p, n, flags, addr, addrlen);
	if (err < 0)
	{
		seterror(err);
		return -1;
	}

	return err; // length
}

int sys_getsockopt(void)
{
	struct file *f;
	int level, optname;
	void *optval;
	int *optlen;

	if (argfd(0, 0, &f) < 0 || argint(1, &level) < 0 || argint(2, &optname) < 0
		|| argptr(3, (void *)&optval, sizeof(*optval)) < 0
		|| argptr(4, (void *)&optlen, sizeof(*optlen)) < 0)
	{
		return -1;
	}

	if (f->type != FD_SOCKET)
	{
		seterror(-ENOTSOCK);
		return -1;
	}

	int err = socketgetsockopt(f->socket, level, optname, optval, optlen);
	if (err < 0)
	{
		seterror(err);
		return -1;
	}

	return err;
}

int sys_setsockopt(void)
{
	struct file *f;
	int level, optname;
	void *optval;
	int optlen;

	if (argfd(0, 0, &f) < 0 || argint(1, &level) < 0 || argint(2, &optname) < 0
		|| argptr(3, (void *)&optval, sizeof(*optval)) < 0
		|| argint(4, &optlen) < 0)
	{
		return -1;
	}

	if (f->type != FD_SOCKET)
	{
		seterror(-ENOTSOCK);
		return -1;
	}

	int err = socksetsockopt(f->socket, level, optname, optval, optlen);
	if (err < 0)
	{
		seterror(err);
		return -1;
	}

	return err;
}
