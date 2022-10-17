//
// Created by bear on 10/4/2022.
//
#include "defs.h"
#include "mmu.h"
#include "spinlock.h"

#include "slab.h"

typedef struct slab
{
	kmem_cache_t *cache;
	size_t in_use;
	size_t next_free;
	void *objects;
	kmem_bufctl *bufctl;

	spinlock_t lock;

	list_head_t link;
} slab_t;

list_head_t caches;
spinlock_t caches_lock;

kmem_cache_t *sized_caches[SIZED_CACHE_COUNT];

kmem_cache_t cache_cache;

static inline size_t cache_object_count(kmem_cache_t *cache, size_t obj_size)
{
	return (PGSIZE - sizeof(slab_t)) / (obj_size + sizeof(kmem_bufctl));
}

static inline slab_t *cache_grow(kmem_cache_t *cache)
{
	assert_holding(&cache->lock);

	char *page = kalloc();
	if (!page)
	{
		return NULL;
	}

	slab_t *slab = (slab_t *)page;

	acquire(&slab->lock);

	slab->cache = cache;
	slab->next_free = 0;
	slab->in_use = 0;
	slab->bufctl = (kmem_bufctl *)(page + sizeof(slab_t));

	char *obj_start = page + sizeof(slab_t) + cache->obj_count * sizeof(kmem_bufctl) + 16;
	obj_start = (char *)PGROUNDUP((uint)obj_start);

	slab->objects = obj_start;
	for (int i = 0; i < cache->obj_count; i++)
	{
		slab->bufctl[i] = i + 1;
	}

	slab->bufctl[cache->obj_count - 1] = -1;

	list_add(&slab->link, &cache->free);

	release(&slab->lock);

	return slab;
}

static inline void slab_destory(slab_t *slab)
{
	assert_holding(&slab->lock);

	list_del(&slab->link);

	kfree((char *)slab);
}

static inline slab_t *slab_find(kmem_cache_t *cache, void *obj)
{
	assert_holding(&cache->lock);

	list_head_t *pos;
	list_for_each(pos, &cache->full)
	{
		slab_t *slab = list_entry(pos, slab_t, link);
		if (obj >= slab->objects && obj < slab->objects + cache->obj_size * cache->obj_count)
		{
			return slab;
		}
	}

	list_for_each(pos, &cache->partial)
	{
		slab_t *slab = list_entry(pos, slab_t, link);
		if (obj >= slab->objects && obj < slab->objects + cache->obj_size * cache->obj_count)
		{
			return slab;
		}
	}

	return NULL;
}

void kmem_init()
{
	cache_cache = (kmem_cache_t){
		.obj_size = sizeof(kmem_cache_t),
		.obj_count = cache_object_count(&cache_cache, sizeof(kmem_cache_t)),
		.flags = 0,
		.name = "cache_cache",
	};

	list_init(&cache_cache.full);
	list_init(&cache_cache.partial);
	list_init(&cache_cache.free);

	list_init(&caches);
	list_add(&cache_cache.link, &caches);

	for (int i = 0; i < SIZED_CACHE_COUNT; i++)
	{
		char name[8] = "sized_";
		name[6] = '0' + i;
		sized_caches[i] = kmem_cache_create(name, 1 << (i + 4), 0);
	}
}

kmem_cache_t *kmem_cache_create(const char *name,
								size_t size,
								uint flags)
{

}

void *kmem_cache_alloc(kmem_cache_t *cache)
{

}

void kmem_cache_destroy(kmem_cache_t *cache)
{

}

void kmem_cache_free(kmem_cache_t *cache, void *obj)
{

}

size_t kmem_cache_shrink(kmem_cache_t *cache)
{

}

size_t kmem_cache_reap()
{

}