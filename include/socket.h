//
// Created by bear on 10/29/2022.
//
#pragma once

#include "sockio.h"
#include "spinlock.h"

#define SOCKET_NBACKLOG 8

#ifdef __KERNEL__
#include "lwip/ip_addr.h"
#else
typedef struct ip4_addr
{
	uint32 addr;
} ip4_addr_t;

typedef ip4_addr_t ip_addr_t;
#endif

typedef struct socket
{
	void *pcb;
	struct pbuf *recv_buf;
	int recv_offset;
	int recv_closed;
	struct socket *backlog[SOCKET_NBACKLOG];
	int protocol;
	int type;

	char connect_chan;
	char accept_chan;
	spinlock_t lock;
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
#define IPPROTO_UDP 1

#define INADDR_ANY 0

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
