//
// Created by bear on 10/20/2022.
//

#include "types.h"
#include "defs.h"
#include "debug.h"
#include "defs.h"
#include "mmu.h"
#include "slab.h"
#include "x86.h"
#include "memlayout.h"

#include "pci.h"
#include "virtio_nic.h"
#include "net.h"

int virtio_net_init(void *state, void *hwaddr);
int virtio_net_send(void *state, const void *data, int len);
int virtio_net_recv(void *state, void *data, int len);

netcard_opts_t virtionet_opts = {
	.init = virtio_net_init,
	.send = virtio_net_send,
	.receive = virtio_net_recv,
};

kmem_cache_t *virtio_cache = NULL;
size_t virtio_count = 0;

int virtio_nic_init(void)
{
	virtio_cache = kmem_cache_create("virtio", sizeof(virtio_device_t), 0);
	if (!virtio_cache)
	{
		panic("virtio_nic_init: kmem_cache_create failed");
	}

	return 0;
}

int virtio_nic_attach(struct pci_func *pcif)
{
	return 0;
}

int virtio_net_init(void *state, void *hwaddr)
{
	return -1;
}

int virtio_net_send(void *state, const void *data, int len)
{
	return -1;
}

int virtio_net_recv(void *state, void *data, int len)
{
	return -1;
}