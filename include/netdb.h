//
// Created by bear on 11/7/2022.
//

#pragma once

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