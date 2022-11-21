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
	uint32 s_addr;
} ip4_addr_t;

typedef ip4_addr_t ip_addr_t;
#endif

#define PF_UNSPEC   0
#define PF_LOCAL    1
#define PF_INET     2

#define AF_UNSPEC   PF_UNSPEC
#define AF_LOCAL    PF_LOCAL
#define AF_INET     PF_INET

#define SOCK_STREAM 1
#define SOCK_DGRAM  2
#define SOCK_RAW    3

#define IPPROTO_IP  0
#define IPPROTO_ICMP 1
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17
#define IPPROTO_RAW 255

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

typedef uint32 in_addr_t;

struct in_addr
{
	in_addr_t s_addr;
};

#define INADDR_ANY (0)
#define INADDR_NONE (0xffffffff)

typedef size_t socklen_t;

#define INET_ADDRSTRLEN  16

typedef struct ifreq
{
	char ifr_name[INET_ADDRSTRLEN]; /* Interface name */
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
		char ifr_slave[INET_ADDRSTRLEN];
		char ifr_newname[INET_ADDRSTRLEN];
		char *ifr_data;
	};
} ifreq_t;

/* For setsockopt(2) */
#define SOL_SOCKET        1

#define SO_DEBUG        1
#define SO_REUSEADDR        2
#define SO_TYPE                3
#define SO_ERROR        4
#define SO_DONTROUTE        5
#define SO_BROADCAST        6
#define SO_SNDBUF        7
#define SO_RCVBUF        8
#define SO_SNDBUFFORCE        32
#define SO_RCVBUFFORCE        33
#define SO_KEEPALIVE        9
#define SO_OOBINLINE        10
#define SO_NO_CHECK        11
#define SO_PRIORITY        12
#define SO_LINGER        13
#define SO_BSDCOMPAT        14
/* To add :#define SO_REUSEPORT 15 */

#ifndef SO_PASSCRED /* powerpc only differs in these */
#define SO_PASSCRED        16
#define SO_PEERCRED        17
#define SO_RCVLOWAT        18
#define SO_SNDLOWAT        19
#define SO_RCVTIMEO        20
#define SO_SNDTIMEO        21
#endif

/* Security levels - as per NRL IPv6 - don't actually do anything */
#define SO_SECURITY_AUTHENTICATION                22
#define SO_SECURITY_ENCRYPTION_TRANSPORT        23
#define SO_SECURITY_ENCRYPTION_NETWORK                24

#define SO_BINDTODEVICE        25

/* Socket filtering */
#define SO_ATTACH_FILTER        26
#define SO_DETACH_FILTER        27

#define SO_PEERNAME                28
#define SO_TIMESTAMP                29
#define SCM_TIMESTAMP                SO_TIMESTAMP

#define SO_ACCEPTCONN                30

#define SO_PEERSEC                31
#define SO_PASSSEC                34
#define SO_TIMESTAMPNS                35
#define SCM_TIMESTAMPNS                SO_TIMESTAMPNS

#define SO_MARK                        36

#define SO_TIMESTAMPING                37
#define SCM_TIMESTAMPING        SO_TIMESTAMPING

#define SO_PROTOCOL                38
#define SO_DOMAIN                39

#define        IP_OPTIONS      4       /* ip_opts; IP per-packet options.  */
#define        IP_HDRINCL      3       /* int; Header is included with data.  */
#define        IP_TOS          1       /* int; IP type of service and precedence.  */
#define        IP_TTL          2       /* int; IP time to live.  */
#define        IP_RECVOPTS     6       /* bool; Receive all IP options w/datagram.  */