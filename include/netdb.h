//
// Created by bear on 11/7/2022.
//

#pragma once

typedef struct netdb_answer
{
	int type;
	struct netdb_answer *next;
} netdb_answer_t;

typedef struct netdb_answer_addr
{
	netdb_answer_t answer;
	uint32 addr;
} netdb_answer_addr_t;

typedef struct netdb_answer_cname_alias_pointer
{
	netdb_answer_t answer;
	char name[256];
} netdb_answer_cname_t, netdb_answer_alias_t, netdb_answer_pointer_t;

typedef enum netdb_answer_type
{
	NETDB_ANSWER_TYPE_ADDR = 1,
	NETDB_ANSWER_TYPE_ALIAS,
	NETDB_ANSWER_TYPE_CANONICAL_NAME,
	NETDB_ANSWER_TYPE_POINTER,
} netdb_answer_type_t;

struct hostent
{
	char *h_name;            /* official name of host */
	char **h_aliases;         /* alias list */
	int h_addrtype;        /* host address type */
	int h_length;          /* length of address */
	char **h_addr_list;       /* list of addresses */
};

#define h_addr h_addr_list[0] /* for backward compatibility */

struct addrinfo
{
	int ai_flags;
	int ai_family;
	int ai_socktype;
	int ai_protocol;
	/*socklen_t*/int ai_addrlen;
	struct sockaddr *ai_addr;
	char *ai_canonname;
	struct addrinfo *ai_next;
};