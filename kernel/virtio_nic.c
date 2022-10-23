//
// Created by bear on 10/20/2022.
//

#include "types.h"
#include "defs.h"

int virtio_nic_attach(struct pci_func *pcif)
{
	return 0;
}

int virtio_net_init(int id, void *hwaddr)
{

}

int virtio_net_send(int id, const void *data, int len)
{

}

int virtio_net_recv(int id, void *data, int len)
{

}