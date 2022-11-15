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

char *netdb_retbuf; // netdb function result buffer of a page's length
char *netdb_qbuf; // query buffer of a page's length
int query_scceeded = -1;

spinlock_t netdb_lock;

struct udp_pcb *dns_pcb = NULL;

// google dns
#define DNS_SERVER 0x08080808

#define DNS_PORT 53

static inline void buf_cleanup()
{
	memset(netdb_retbuf, 0, PGSIZE);
	memset(netdb_qbuf, 0, PGSIZE);
}

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
	uint16_t compression;
	uint16_t type;
	uint16_t class;
	uint32_t ttl;
	uint16_t length;
}__attribute__((packed)) dns_record_header_t;

/* Structure of the bytes for an IPv4 answer */
typedef struct dns_record_a
{
	dns_record_header_t header;
	ip_addr_t addr;
} __attribute__((packed)) dns_record_a_t;

typedef struct dns_record_cname
{
	dns_record_header_t header;
	char name[0];
} __attribute__((packed)) dns_record_cname_t;

#define TYPE_A 1
#define TYPE_CNAME 5

static inline void header_ntoh(dns_record_header_t *h)
{
	h->compression = ntohs(h->compression);
	h->type = ntohs(h->type);
	h->class = ntohs(h->class);
	h->ttl = ntohl(h->ttl);
	h->length = ntohs(h->length);
}

static inline char *parse_dns_name(char *str, char *data)
{
	uint8_t *start_of_name = (uint8_t *)str;
	uint8_t total = 0;
	uint8_t *field_length = start_of_name;
	while (*field_length != 0 && *field_length != 0xC0)
	{
		/* Restore the dot in the name and advance to next length */
		total += *field_length + 1;
		*field_length = '.';
		field_length = start_of_name + total;
	}

	for (uint8 *p = start_of_name; p <= field_length; p++)
	{
		if (((*p) & 0xF0) == 0xC0)
		{
			uint16 offset = ntohs(*((uint16 *)(p))) & 0x0FFF;
			char *i = data + offset;
			while (*i)
			{
				*p++ = *i++;
			}
			*p = 0;
		}
	}

	return (char *)(start_of_name + 1);
}

static inline void fill_result(dns_header_t *header, char *data)
{
	struct hostent *host = (struct hostent *)netdb_retbuf;
	char *r = netdb_retbuf + sizeof(struct hostent);

	size_t alias_count = 0, addr_count = 0;
	size_t cannonical_name_len = 0;
	const int ans_count = ntohs(header->ancount);

	char *p = data;
	for (int i = 0; i < ans_count; i++)
	{
		dns_record_header_t *record_header = (dns_record_header_t *)p;
		header_ntoh(record_header);
		if (record_header->type == TYPE_A)
		{
			dns_record_a_t *record = (dns_record_a_t *)record_header;
			int offset = record->header.compression & 0x0FFF;
			char *can_name = ((char *)header) + offset;
			can_name = parse_dns_name(can_name, (char *)header);
			cannonical_name_len = strlen(can_name) + 1;
			addr_count++;
		}
		else if (record_header->type == TYPE_CNAME)
		{
			alias_count++;
		}

		p += sizeof(dns_record_header_t) + record_header->length;
	}

	host->h_aliases = (char **)r;
	r += (alias_count + 1) * sizeof(char *);
	host->h_addr_list = (char **)r;
	r += (addr_count + 1) * sizeof(char *);
	host->h_name = r;
	r += cannonical_name_len + 1;

	int cname_pos = 0, ip_pos = 0;
	for (int i = 0; i < ans_count; i++)
	{
		dns_record_header_t *record_header = (dns_record_header_t *)data;
		if (ntohs(record_header->type) == TYPE_A)
		{
			dns_record_a_t *record = (dns_record_a_t *)data;
			host->h_addr_list[ip_pos++] = r;
			memmove(r, &record->addr, sizeof(ip_addr_t));
			r += sizeof(ip_addr_t);
			int offset = record->header.compression & 0x0FFF;
			memmove(host->h_name, ((char *)header) + offset, cannonical_name_len);
		}
		else if (ntohs(record_header->type) == TYPE_CNAME)
		{
			char *cname = ((dns_record_cname_t *)data)->name;
			strncpy(r, cname, strlen(cname));
			host->h_aliases[cname_pos++] = r;
			r += strlen(cname) + 1;
		}
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
		// if response has error or server cannot do recursive query, the query will fail.
		query_scceeded = 1;
		goto end;
	}

	uint8_t *start_of_name = (uint8_t * )(netdb_qbuf + sizeof(dns_header_t));
	uint8_t total = 0;
	uint8_t *field_length = start_of_name;
	while (*field_length != 0)
	{
		/* Restore the dot in the name and advance to next length */
		total += *field_length + 1;
		*field_length = '.';
		field_length = start_of_name + total;
	}

	fill_result(response_header, (char *)field_length + 5);
	query_scceeded = 0;
end:
	pbuf_free(p);
	wakeup(netdb_retbuf);
}

void netdbinit(void)
{
	netdb_retbuf = page_alloc();
	KDEBUG_ASSERT(netdb_retbuf != NULL);
	netdb_qbuf = page_alloc();
	KDEBUG_ASSERT(netdb_qbuf != NULL);
	buf_cleanup();

	initlock(&netdb_lock, "netdb");

	netbegin_op();

	dns_pcb = udp_new();
	KDEBUG_MSG_ASSERT(dns_pcb != NULL, "dns_pcb is NULL");

	struct ip4_addr dns_server = {DNS_SERVER};
	err_t err = udp_connect(dns_pcb, &dns_server, DNS_PORT);
	KDEBUG_MSG_ASSERT(err == ERR_OK, "udp_bind failed");

	netend_op();

	cprintf("netdbinit: netdb_retbuf = 0x%p\n", netdb_retbuf);
}

struct hostent *gethostbyname(const char *hostname)
{
	struct hostent *host = (struct hostent *)netdb_retbuf;
	acquire(&netdb_lock);
	buf_cleanup();

	char *p = netdb_retbuf; // temporarily use
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

	const uint8 hostlen = (uint8)strlen(hostname);
	memmove(question->name + 1, hostname, hostlen + 1);

	uint8 *prev = (uint8 *)question->name;
	uint8 count = 0;

	for (uint8 i = 0; i < hostlen; i++)
	{
		if (hostname[i] == '.')
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

	size_t packet_len = sizeof(dns_header_t) + sizeof(dns_question_t) - sizeof(char *) + hostlen + 2;
	p = netdb_qbuf;

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
	query_scceeded = -1;
	udp_recv(dns_pcb, netdb_dns_recv, netdb_retbuf);
	err_t err = udp_send(dns_pcb, pbuf);

	if (err != ERR_OK)
	{
		netend_op();
		pbuf_free(pbuf);
		return NULL;
	}
	netend_op();

	if (query_scceeded < 0)
	{
		sleep(netdb_retbuf, &netdb_lock);
	}
	release(&netdb_lock);

	if (query_scceeded != 0)
	{
		return NULL;
	}

	return host;
}

struct hostent *gethostbyaddr(const char *addr, int len, int type)
{
	return NULL;
}



