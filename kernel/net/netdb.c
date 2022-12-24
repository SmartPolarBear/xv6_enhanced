//
// Created by bear on 11/7/2022.
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

#include "internal/netdb.h"

extern netdb_proto_t doh_proto;
extern netdb_proto_t bare_dns_proto;

static netdb_proto_t *netdb_protos[] = {
	&bare_dns_proto,
	&doh_proto,
};

spinlock_t netdb_lock;

void netdbinit(void)
{
	initlock(&netdb_lock, "netdb");

	for (int i = 0; i < NELEM(netdb_protos); i++)
	{
		netdb_protos[i]->proto_init();
	}
}

struct netdb_answer *netdb_query(char *name, int type)
{
	acquire(&netdb_lock);

	struct netdb_answer *ans = NULL;
	for (int i = 0; i < NELEM(netdb_protos); i++)
	{
		int ret = netdb_protos[i]->proto_query(name, type, &ans);
		if (ret == 0)
		{
			break;
		}
	}
	release(&netdb_lock);

	return ans;
}

void netdb_free(struct netdb_answer *ans)
{
	acquire(&netdb_lock);

	struct netdb_answer *p = ans;
	while (p)
	{
		struct netdb_answer *next = p->next;

		for (int i = 0; i < NELEM(netdb_protos); i++)
		{
			int ret = netdb_protos[i]->proto_free_answer(p);
			if (ret == 0)
			{
				break;
			}
		}

		p = next;
	}

	release(&netdb_lock);
}

void netdb_dump_answer(struct netdb_answer *ans)
{
	acquire(&netdb_lock);
	
	struct netdb_answer *p = ans;
	while (p)
	{
		for (int i = 0; i < NELEM(netdb_protos); i++)
		{
			int ret = netdb_protos[i]->proto_dump_answer(p);
			if (ret == 0)
			{
				break;
			}
		}
		p = p->next;
	}

	release(&netdb_lock);
}
