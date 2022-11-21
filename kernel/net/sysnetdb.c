//
// Created by bear on 11/7/2022.
//

#include "types.h"
#include "defs.h"
#include "netdb.h"
#include "mmu.h"
#include "socket.h"
#include "param.h"
#include "proc.h"
#include "memlayout.h"

int sys_gethostbyname(void)
{
	char *name = NULL;
	if (argstr(0, &name) < 0)
	{
		return -1;
	}
	struct netdb_answer *ans = netdb_query(name, QUERY_DNS);

	if (!ans)
	{
		return 0;
	}

	char *buf = myproc()->inet.netdb;
	if (!buf)
	{
		buf = myproc()->inet.netdb = page_alloc();
		if (remappages(myproc()->pgdir, (void *)buf, PGSIZE, V2P(buf), PTE_W | PTE_U) < 0)
		{
			return 0;
		}
	}
	memset(buf, 0, PGSIZE);

	struct hostent *host = (struct hostent *)buf;
	char *p = buf + sizeof(struct hostent);
	host->h_name = p;

	int addr_count = 0, alias_count = 0;
	for (netdb_answer_t *it = ans; it; it = it->next)
	{
		if (it->type == NETDB_ANSWER_TYPE_CANONICAL_NAME)
		{
			netdb_answer_cname_t *cname = (netdb_answer_cname_t *)it;
			strncpy(p, cname->name, 256);
			p += strlen(cname->name) + 1;
		}
		else if (it->type == NETDB_ANSWER_TYPE_ALIAS)
		{
			alias_count++;
		}
		else if (it->type == NETDB_ANSWER_TYPE_ADDR)
		{
			addr_count++;
		}
	}

	char **alias_list = (char **)p;
	p += (alias_count + 1) * sizeof(char *);
	char **addr_list = (char **)p;
	p += (addr_count + 1) * sizeof(char *);

	host->h_aliases = alias_list;
	host->h_addr_list = addr_list;

	for (netdb_answer_t *it = ans; it; it = it->next)
	{
		if (it->type == NETDB_ANSWER_TYPE_ALIAS)
		{
			netdb_answer_alias_t *alias = (netdb_answer_alias_t *)it;
			*alias_list++ = p;
			strncpy(p, alias->name, 256);
			p += strlen(alias->name) + 1;
		}
		else if (it->type == NETDB_ANSWER_TYPE_ADDR)
		{
			netdb_answer_addr_t *addr = (netdb_answer_addr_t *)it;
			*addr_list++ = p;
			*(uint32 *)p = addr->addr;
			p += 4;
		}
	}

//	netdb_dump_answer(ans);
	netdb_free(ans);

	host->h_addrtype = AF_INET;
	host->h_length = 4;

	return (int)host;
}

int sys_gethostbyaddr(void)
{
	return 0;
}