//
// Created by bear on 11/16/2022.
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
#include "netdb.h"
#include "socket.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/dns.h"

int
inet_aton(const char *cp, struct in_addr *ap)
{
	int dots = 0;
	register uint32 acc = 0, addr = 0;

	do
	{
		register char cc = *cp;

		switch (cc)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			acc = acc * 10 + (cc - '0');
			break;

		case '.':
			if (++dots > 3)
			{
				return 0;
			}
			/* Fall through */

		case '\0':
			if (acc > 255)
			{
				return 0;
			}
			addr = addr << 8 | acc;
			acc = 0;
			break;

		default:
			return 0;
		}
	} while (*cp++);

	/* Normalize the address */
	if (dots < 3)
	{
		addr <<= 8 * (3 - dots);
	}

	/* Store it if requested */
	if (ap)
	{
		ap->s_addr = htonl(addr);
	}

	return 1;
}

static char buf[18];

char *
inet_ntoa(struct in_addr in)
{
	char *b = buf;
	register char *p;

	p = (char *)&in;
#define    UC(b)    (((int)b)&0xff)
	for (int i = 0; i < 4; i++)
	{
		char val = UC(p[i]);
		if (val >= 100)
		{
			*b++ = '0' + val / 100;
			val %= 100;
			*b++ = '0' + val / 10;
			val %= 10;
		}
		else if (val >= 10)
		{
			*b++ = '0' + val / 10;
			val %= 10;
		}
		*b++ = '0' + val;
		*b++ = '.';
	}

	return buf;
}