//
// Created by bear on 10/24/2022.
//
#pragma once

#include "types.h"
#include "if.h"

#define NNETCARDNAME 16

#define NETDEV_FLAG_BROADCAST IFF_BROADCAST
#define NETDEV_FLAG_MULTICAST IFF_MULTICAST
#define NETDEV_FLAG_P2P       IFF_POINTOPOINT
#define NETDEV_FLAG_LOOPBACK  IFF_LOOPBACK
#define NETDEV_FLAG_NOARP     IFF_NOARP
#define NETDEV_FLAG_PROMISC   IFF_PROMISC
#define NETDEV_FLAG_RUNNING   IFF_RUNNING
#define NETDEV_FLAG_UP        IFF_UP

typedef struct netcard_opts
{
	int (*init)(void *, void *);
	int (*open)(void *);
	int (*stop)(void *);
	int (*send)(void *, const void *, int);
	int (*receive)(void *, void *, int);
} netcard_opts_t;

typedef struct netdev
{
	char name[NNETCARDNAME];
	list_head_t link;

	struct pci_func *func;
	struct netcard_opts *opts;

	uint32 id;
	uint32 flags;

	void *priv;
} netdev_t;