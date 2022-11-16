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

typedef struct
{
	uint16 xid;      /* Randomly chosen identifier */
	uint16 flags;    /* Bit-mask to indicate request/response */
	uint16 qdcount;  /* Number of questions */
	uint16 ancount;  /* Number of answers */
	uint16 nscount;  /* Number of authority records */
	uint16 arcount;  /* Number of additional records */
} dns_header_t;

typedef struct
{
	char *name;        /* Pointer to the domain name in memory */
	uint16 dnstype;  /* The QTYPE (1 = A) */
	uint16 dnsclass; /* The QCLASS (1 = IN) */
} dns_question_t;

typedef struct dns_record_header
{
	uint16 compression;
	uint16 type;
	uint16 class;
	uint32 ttl;
	uint16 length;
}__attribute__((packed)) dns_record_header_t;

/* Structure of the bytes for an IPv4 answer */
typedef struct dns_record_a
{
	dns_record_header_t header;
	unsigned long addr;
} __attribute__((packed)) dns_record_a_t;

typedef struct dns_record_cname_ptr
{
	dns_record_header_t header;
	char name[0];
} __attribute__((packed)) dns_record_cname_t, dns_record_ptr_t;

#define TYPE_A 1
#define TYPE_PTR 12
#define TYPE_CNAME 5

char *netdb_qbuf;
struct netdb_answer *query_ans;

spinlock_t netdb_lock;

kmem_cache_t *addr_cache;
kmem_cache_t *name_cache;

struct udp_pcb *dns_pcb = NULL;

// google dns
#define DNS_SERVER 0x08080808
#define DNS_PORT 53

static inline void buf_cleanup()
{
	memset(netdb_qbuf, 0, PGSIZE);
}

static inline netdb_answer_cname_t *alloc_canno_name_answer()
{
	netdb_answer_cname_t *answer = kmem_cache_alloc(name_cache);
	KDEBUG_ASSERT(answer != NULL);
	memset(answer, 0, sizeof(netdb_answer_cname_t));
	answer->answer.type = NETDB_ANSWER_TYPE_CANONICAL_NAME;
	return answer;
}

static inline netdb_answer_alias_t *alloc_alias_answer()
{
	netdb_answer_alias_t *answer = kmem_cache_alloc(name_cache);
	KDEBUG_ASSERT(answer != NULL);
	memset(answer, 0, sizeof(netdb_answer_alias_t));
	answer->answer.type = NETDB_ANSWER_TYPE_ALIAS;
	return answer;
}

static inline netdb_answer_addr_t *alloc_addr_answer()
{
	netdb_answer_addr_t *answer = kmem_cache_alloc(addr_cache);
	KDEBUG_ASSERT(answer != NULL);
	memset(answer, 0, sizeof(netdb_answer_addr_t));
	answer->answer.type = NETDB_ANSWER_TYPE_ADDR;
	return answer;
}

/// decompression dns name
/// \param dst the result
/// \param name compressed name
/// \param ans full response data
static inline char *parse_dns_name(char *dst, char *name, char *ans)
{
	char *p = name;
	char *d = dst;
	char *s = ans;
	int len = 0;
	int i = 0;
	int offset = 0;
	int first = 1;

	while (*p != 0)
	{
		if ((*p & 0xc0) == 0xc0)
		{
			offset = (*p & 0x3f) << 8;
			p++;
			offset |= *p;
			p = s + offset;
			continue;
		}

		len = *p;
		p++;

		if (first)
		{
			first = 0;
		}
		else
		{
			*d = '.';
			d++;
		}

		for (i = 0; i < len; i++)
		{
			*d = *p;
			d++;
			p++;
		}
	}

	*d = 0;

	return p + 1;
}

void append_answer(netdb_answer_t **answer, netdb_answer_t *new_answer)
{
	if (*answer == NULL)
	{
		*answer = new_answer;
		return;
	}

	netdb_answer_t *p = *answer;
	while (p->next)
	{
		p = p->next;
	}
	p->next = new_answer;
}

static inline void parse_result(dns_header_t *header, char *data)
{
	const int ans_count = ntohs(header->ancount);
	int has_canonical = FALSE;

	for (int i = 0; i < ans_count; i++)
	{
		dns_record_header_t *record_header = (dns_record_header_t *)data;
		if (ntohs(record_header->type) == TYPE_A)
		{
			dns_record_a_t *record = (dns_record_a_t *)data;
			netdb_answer_addr_t *answer = alloc_addr_answer();
			answer->addr = ntohl(record->addr);
			append_answer(&query_ans, (netdb_answer_t *)answer);

			if (!has_canonical)
			{
				has_canonical = TRUE;
				netdb_answer_cname_t *cname = alloc_canno_name_answer();
				int offset = htons(record->header.compression) & 0x0FFF;
				char *can_name = ((char *)header) + offset;
				parse_dns_name(cname->name, can_name, (char *)header);
				append_answer(&query_ans, (netdb_answer_t *)cname);
			}
		}
		else if (ntohs(record_header->type) == TYPE_CNAME ||
			ntohs(record_header->type) == TYPE_PTR)
		{
			char *cname = ((dns_record_cname_t *)data)->name;
			netdb_answer_alias_t *answer = alloc_alias_answer();
			parse_dns_name(answer->name, cname, (char *)header);
			append_answer(&query_ans, (netdb_answer_t *)answer);
		}

		data += sizeof(dns_record_header_t) + ntohs(record_header->length);
	}
}

void netdb_dns_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p,
					const ip_addr_t *addr, u16_t port)
{
	pbuf_copy_partial(p, netdb_qbuf, p->tot_len, 0);

	dns_header_t *response_header = (dns_header_t *)netdb_qbuf;
	uint16 response_flags = ntohs(response_header->flags);
	if ((response_flags & 0xf) != 0 || (response_flags & 0x80) != 0x80)
	{
		return;
	}

	uint8_t *start_of_name = (uint8_t * )(netdb_qbuf + sizeof(dns_header_t));
	uint8_t total = 0;
	uint8_t *field_length = start_of_name;
	while (*field_length != 0)
	{
		/* Restore the dot in the name and advance to next length */
		total += *field_length + 1;
		field_length = start_of_name + total;
	}

	parse_result(response_header, (char *)field_length + 5);

	pbuf_free(p);
	wakeup(&dns_pcb);
}

void netdbinit(void)
{
	netdb_qbuf = page_alloc();
	KDEBUG_ASSERT(netdb_qbuf != NULL);
	buf_cleanup();

	addr_cache = kmem_cache_create("netdbaddr", sizeof(netdb_answer_addr_t), 0);
	name_cache = kmem_cache_create("netdbname", sizeof(netdb_answer_cname_t), 0);

	initlock(&netdb_lock, "netdb");

	netbegin_op();

	dns_pcb = udp_new();
	KDEBUG_MSG_ASSERT(dns_pcb != NULL, "dns_pcb is NULL");

	struct ip4_addr dns_server = {DNS_SERVER};
	err_t err = udp_connect(dns_pcb, &dns_server, DNS_PORT);
	KDEBUG_MSG_ASSERT(err == ERR_OK, "udp_bind failed");

	netend_op();
}

static inline struct netdb_answer *make_query(char *name)
{
	assert_holding(&netdb_lock);

	char *p = netdb_qbuf + 2048;

	dns_header_t *header = (dns_header_t *)p;
	header->xid = byteswap16(0x1204); // should be random though
	header->flags = byteswap16(0x0100); // Q, RD
	header->qdcount = byteswap16(1); // 1 question

	p += sizeof(dns_header_t);
	dns_question_t *question = (dns_question_t *)p;
	question->dnstype = byteswap16(1); // A
	question->dnsclass = byteswap16(1); // IN

	p += sizeof(dns_question_t);
	question->name = p;

	const uint8 hostlen = (uint8)strlen(name);
	memmove(question->name + 1, name, hostlen + 1);

	uint8 *prev = (uint8 *)question->name;
	uint8 count = 0;

	for (uint8 i = 0; i < hostlen; i++)
	{
		if (name[i] == '.')
		{
			*prev = count;
			prev = (uint8 *)(question->name + i + 1);
			count = 0;
		}
		else
		{
			count++;
		}
	}
	*prev = count;

	p = netdb_qbuf;
	udp_send(dns_pcb, pbuf_alloc(PBUF_TRANSPORT, p - netdb_qbuf, PBUF_RAM));
	size_t packet_len = sizeof(dns_header_t) + sizeof(dns_question_t) - sizeof(char *) + hostlen + 2;

	memmove(p, header, sizeof(dns_header_t));
	p += sizeof(dns_header_t);

	memmove(p, question->name, hostlen + 1);
	p += hostlen + 2;

	memmove(p, &question->dnstype, sizeof(question->dnstype));
	p += sizeof(question->dnstype);
	memmove(p, &question->dnsclass, sizeof(question->dnsclass));

	netbegin_op();
	struct pbuf *pbuf = pbuf_alloc(PBUF_TRANSPORT, packet_len, PBUF_RAM);
	if (!pbuf)
	{
		netend_op();
		return NULL;
	}
	memmove(pbuf->payload, netdb_qbuf, packet_len);
	query_ans = NULL;
	udp_recv(dns_pcb, netdb_dns_recv, NULL);
	err_t err = udp_send(dns_pcb, pbuf);

	if (err != ERR_OK)
	{
		netend_op();
		pbuf_free(pbuf);
		return NULL;
	}
	netend_op();

	if (!query_ans)
	{
		sleep(&dns_pcb, &netdb_lock);
	}
	return query_ans;
}

struct netdb_answer *netdb_query(char *name, int type)
{
	acquire(&netdb_lock);
	buf_cleanup();

	struct netdb_answer *ans = NULL;
	if (type == 0) // DNS
	{
		ans = make_query(name);
	}
	else // reverse DNS
	{
		char *p = netdb_qbuf + 3072;
		strncpy(p, name, 512);
		char *iter = p;
		while (*iter)
			iter++;
		strncpy(iter, ".in-addr.arpa", 14);

		ans = make_query(p);
	}

	release(&netdb_lock);
	return ans;
}

void netdb_free(struct netdb_answer *ans)
{
	struct netdb_answer *p = ans;
	while (p)
	{
		struct netdb_answer *next = p->next;
		if (p->type == NETDB_ANSWER_TYPE_ADDR)
		{
			kmem_cache_free(addr_cache, p);
		}
		else if (p->type == NETDB_ANSWER_TYPE_ALIAS || p->type == NETDB_ANSWER_TYPE_CANONICAL_NAME)
		{
			kmem_cache_free(name_cache, p);
		}
		else
		{
			panic("unknown netdb type");
		}
		p = next;
	}
}

void netdb_dump_answer(struct netdb_answer *ans)
{
	struct netdb_answer *p = ans;
	while (p)
	{
		if (p->type == NETDB_ANSWER_TYPE_ADDR)
		{
			netdb_answer_addr_t *addr = (netdb_answer_addr_t *)p;
			cprintf("addr: 0x%x\n", addr->addr);
		}
		else if (p->type == NETDB_ANSWER_TYPE_ALIAS)
		{
			netdb_answer_cname_t *cname = (netdb_answer_cname_t *)p;
			cprintf("alias: %s\n", cname->name);
		}
		else if (p->type == NETDB_ANSWER_TYPE_CANONICAL_NAME)
		{
			netdb_answer_cname_t *cname = (netdb_answer_cname_t *)p;
			cprintf("cname: %s\n", cname->name);
		}
		else
		{
			panic("unknown netdb type");
		}
		p = p->next;
	}
}
