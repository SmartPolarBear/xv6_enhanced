//
// Created by bear on 11/17/2022.
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

typedef enum netdb_query_type
{
	QUERY_DNS = 0,
	QUERY_ARPA,
} netdb_query_type_t;