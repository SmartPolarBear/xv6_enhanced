//
// Created by bear on 10/17/2022.
//

#include "memory.h"
#include "slab.h"
#include "mmu.h"
#include "debug.h"

enum kmalloc_type
{
	KML_SLAB, KML_PAGE
};

typedef struct kmallocctl
{
	size_t size;
	uint type;
	union
	{
		kmem_cache_t *cache;
		int page_count;
	};
	char buf[0];
} kmallocctl_t;

void *kmalloc(size_t size, gfp_t flags)
{
	size += sizeof(kmallocctl_t);

	// temporarily disable allocation larger than 1 page
	KDEBUG_ASSERT(size <= PGSIZE);

	kmallocctl_t *buf = NULL;
	if (size <= SIZED_CACHE_MAX)
	{
		size_t index = 0;
		while (SIZED_INDEX_TO_SIZE(index) < size)
		{
			index++;
		}

		buf = (kmallocctl_t *)kmem_cache_alloc(sized_caches[index]);
		if (!buf)
		{
			return NULL;
		}

		buf->cache = sized_caches[index];
		buf->type = KML_SLAB;
	}
	else
	{
		buf = (kmallocctl_t *)page_alloc();
		if (!buf)
		{
			return NULL;
		}

		buf->page_count = 1;
		buf->type = KML_PAGE;
	}

	return buf->buf;
}

void kfree(void *ptr)
{
	kmallocctl_t *buf = (kmallocctl_t *)((char *)ptr - sizeof(kmallocctl_t));

	if (buf->type == KML_SLAB)
	{
		kmem_cache_free(buf->cache, buf);
	}
	else
	{
		page_free((char *)buf);
	}
}
