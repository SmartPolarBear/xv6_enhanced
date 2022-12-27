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

static const unsigned char cert[] = {"-----BEGIN CERTIFICATE-----\r\n"
									 "MIIF4zCCBMugAwIBAgIQDuFP5A30dqAKdE3RO/TpSDANBgkqhkiG9w0BAQsFADBG\r\n"
									 "MQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExM\r\n"
									 "QzETMBEGA1UEAxMKR1RTIENBIDFDMzAeFw0yMjExMDIxMzQ1NDJaFw0yMzAxMjUx\r\n"
									 "MzQ1NDFaMBUxEzARBgNVBAMTCmRucy5nb29nbGUwggEiMA0GCSqGSIb3DQEBAQUA\r\n"
									 "A4IBDwAwggEKAoIBAQDAyEjZ7ZGRpgR80UlplpawUwSSevq1XZ9wHGg2/kunTJBx\r\n"
									 "lVZEg2wse/3K5B2//Gfxu1FRgtEg7JZJAXqgF9FY2u2iZGPdGQG1CfybP8woJ2GF\r\n"
									 "QgkA4tAMO5XEMpwHh4C5uJhbmmXE1Qg54/iEGv9nBP5nUw1LRKrT5x3hLAc9MbE/\r\n"
									 "SjmBSZ0fTJDW7O7kpCTt8A2eY9vWkG+5OfGjVHTU1VyR1hgp5TSU+wGcRM/Ww+PF\r\n"
									 "r1VrshbG1XcN+XIeFWLIBgtUifcUYniuFRQGEGeQ1y4ukaKop37QjYMkaeSKMlqE\r\n"
									 "ubwCTNPHHqy6ztjM3Uk8Mkjdoo+Y2SnGIlQY/C4NAgMBAAGjggL8MIIC+DAOBgNV\r\n"
									 "HQ8BAf8EBAMCBaAwEwYDVR0lBAwwCgYIKwYBBQUHAwEwDAYDVR0TAQH/BAIwADAd\r\n"
									 "BgNVHQ4EFgQUOdY4iCM3bp4Day6YVnvOR9v6K1QwHwYDVR0jBBgwFoAUinR/r4XN\r\n"
									 "7pXNPZzQ4kYU83E1HScwagYIKwYBBQUHAQEEXjBcMCcGCCsGAQUFBzABhhtodHRw\r\n"
									 "Oi8vb2NzcC5wa2kuZ29vZy9ndHMxYzMwMQYIKwYBBQUHMAKGJWh0dHA6Ly9wa2ku\r\n"
									 "Z29vZy9yZXBvL2NlcnRzL2d0czFjMy5kZXIwgawGA1UdEQSBpDCBoYIKZG5zLmdv\r\n"
									 "b2dsZYIOZG5zLmdvb2dsZS5jb22CECouZG5zLmdvb2dsZS5jb22CCzg4ODguZ29v\r\n"
									 "Z2xlghBkbnM2NC5kbnMuZ29vZ2xlhwQICAgIhwQICAQEhxAgAUhgSGAAAAAAAAAA\r\n"
									 "AIiIhxAgAUhgSGAAAAAAAAAAAIhEhxAgAUhgSGAAAAAAAAAAAGRkhxAgAUhgSGAA\r\n"
									 "AAAAAAAAAABkMCEGA1UdIAQaMBgwCAYGZ4EMAQIBMAwGCisGAQQB1nkCBQMwPAYD\r\n"
									 "VR0fBDUwMzAxoC+gLYYraHR0cDovL2NybHMucGtpLmdvb2cvZ3RzMWMzL1FxRnhi\r\n"
									 "aTlNNDhjLmNybDCCAQUGCisGAQQB1nkCBAIEgfYEgfMA8QB2AOg+0No+9QY1MudX\r\n"
									 "KLyJa8kD08vREWvs62nhd31tBr1uAAABhDjOJhsAAAQDAEcwRQIgPuOeBMnXjE8C\r\n"
									 "UF9YGGXsMBCD7PVbXhTrmiJ5v5JW4TQCIQCy5WNY1z6kgBdSssCc9Vqs7VxtVE1Q\r\n"
									 "L8bf93NDCPm/kwB3AHoyjFTYty22IOo44FIe6YQWcDIThU070ivBOlejUutSAAAB\r\n"
									 "hDjOJnIAAAQDAEgwRgIhAIcm5tarfDPRhSEYEON/LO2DiyZQuozCZ2M5CzGz0BKw\r\n"
									 "AiEA4S1W6EX3Vctd9dbWzrWb82qHQgVPgkM7m9vSS/cQlzswDQYJKoZIhvcNAQEL\r\n"
									 "BQADggEBAJaee8gwW0H5jHb+hqhkccDcqnFUWQcPvMAX5zpSqM+z6vDpy/elvbsN\r\n"
									 "U6nsiYaT5yEbDr5nAeeGuDbyqiK3VgmibNWyLxmS8HiOx7Zm/vIq4FhBhaqTTk/3\r\n"
									 "0fYock09d7Fg0WPN9sRg0GsVfgc/jIWuygGJeZ/qWW0/6Qs1crCzeHhS+HqzfQM8\r\n"
									 "bn5ycvcndnxNHtMCP+YB8utAoafn2+UYX/oA1xq8CaF10HXNlqc3DTTZ/benRK3Y\r\n"
									 "1wjHo9f9Q+BVMCGwWvDID+ranzUg7XoZd71dc8dzFP1oXBBQJBoBVmqIY1xvGzof\r\n"
									 "/fTvNpHS+YYkJQQ7w7aq8FxoAdVZwZY=\r\n"
									 "-----END CERTIFICATE-----\r\n"};

const ip_addr_t google_dns_ip = IPADDR4_INIT_BYTES(8, 8, 8, 8);

struct altcp_pcb *dohpcb = NULL;

err_t doh_connected(void *arg, struct altcp_pcb *conn, err_t err)
{
	if (err != ERR_OK)
	{
		cprintf("doh_connected: error %d", err);
		KDEBUG_ASSERT(0);
	}
	cprintf("DoH: connected to the server");
	return ERR_OK;
}

void doh_init()
{
//	return;
	struct altcp_tls_config *conf = altcp_tls_create_config_client(NULL, sizeof(cert));
	KDEBUG_MSG_ASSERT(conf, "altcp_tls_create_config_client failed");
	altcp_allocator_t tls_allocator = {
		altcp_tls_alloc, conf
	};
	dohpcb = altcp_new(&tls_allocator);
	KDEBUG_MSG_ASSERT(dohpcb, "altcp_new failed");
	err_t err = altcp_connect(dohpcb, &google_dns_ip, 443, doh_connected);
	KDEBUG_MSG_ASSERT(err == ERR_OK, "altcp_connect failed");
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