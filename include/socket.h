//
// Created by bear on 10/29/2022.
//
#pragma once

#include "sockio.h"
#include "lwip/ip_addr.h"

typedef struct socket
{
	int type;
	int desc;
} socket_t;

#define PF_UNSPEC   0
#define PF_LOCAL    1
#define PF_INET     2

#define AF_UNSPEC   PF_UNSPEC
#define AF_LOCAL    PF_LOCAL
#define AF_INET     PF_INET

#define SOCK_STREAM 1
#define SOCK_DGRAM  2

#define IPPROTO_TCP 0
#define IPPROTO_UDP 0

#define INADDR_ANY ((ip_addr_t)0)

typedef struct sockaddr
{
	uint16 sa_family;
	uint8 sa_data[14];
} sockaddr_t;

typedef struct sockaddr_in
{
	uint16 sin_family;
	uint16 sin_port;
	ip_addr_t sin_addr;
} sockaddr_in_t;

#define IFNAMSIZ 16

typedef struct ifreq
{
	char ifr_name[IFNAMSIZ]; /* Interface name */
	union
	{
		struct sockaddr ifr_addr;
		struct sockaddr ifr_dstaddr;
		struct sockaddr ifr_broadaddr;
		struct sockaddr ifr_netmask;
		struct sockaddr ifr_hwaddr;
		short ifr_flags;
		int ifr_ifindex;
		int ifr_metric;
		int ifr_mtu;
//      struct ifmap    ifr_map;
		char ifr_slave[IFNAMSIZ];
		char ifr_newname[IFNAMSIZ];
		char *ifr_data;
	};
} ifreq_t;
