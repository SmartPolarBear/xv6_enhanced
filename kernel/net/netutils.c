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

static char buf[INET_ADDRSTRLEN];

char *inet_ntoa(struct in_addr in)
{
	uint8 *p = (uint8 *)&in.s_addr;
	memset(buf, 0, sizeof(buf));
	snprintf(buf, INET_ADDRSTRLEN, "%d.%d.%d.%d", p[3], p[2], p[1], p[0]);
	return buf;
}