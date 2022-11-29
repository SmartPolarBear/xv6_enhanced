//
// Created by bear on 11/23/2022.
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

#include "netdb.h"
#include "lwip/altcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/altcp_tcp.h"

#include "internal/netdb.h"

//static const unsigned char cert[] = {0};

void doh_init()
{
//	struct altcp_tls_config *conf = altcp_tls_create_config_client(cert, sizeof(cert));
//	altcp_allocator_t tls_allocator = {
//		altcp_tls_alloc, conf
//	};
}

int doh_query(char *name, netdb_query_type_t type, netdb_answer_t **answer)
{
	return -1;
}

int doh_free_answer(netdb_answer_t *answer)
{
	return -1;
}

int doh_dump_answer(const netdb_answer_t *answer)
{
	return -1;
}

void doh_shutdown()
{

}

netdb_proto_t doh_proto = {
	.proto_init = doh_init,
	.proto_query = doh_query,
	.proto_free_answer = doh_free_answer,
	.proto_dump_answer=doh_dump_answer,
	.proto_shutdown = doh_shutdown,
};