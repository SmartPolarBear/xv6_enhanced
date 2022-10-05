//
// Created by bear on 10/4/2022.
//

#include "slab.h"

void kmem_init()
{

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