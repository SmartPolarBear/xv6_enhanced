#pragma once

#include "spinlock.h"
#include "types.h"
#include "list.h"

#define KMEM_CACHE_NAME_MAXLEN 16
#define SIZED_CACHE_COUNT 8

#define SIZED_INDEX_TO_SIZE(index) (1 << (4 + (index)))

#define SIZED_CACHE_MIN SIZED_INDEX_TO_SIZE(0)
#define SIZED_CACHE_MAX SIZED_INDEX_TO_SIZE(SIZED_CACHE_COUNT-1)

typedef struct kmem_cache
{
	list_head_t full, partial, free;

	list_head_t link;

	size_t obj_size, obj_count;
	uint flags;
	spinlock_t lock;

	char name[KMEM_CACHE_NAME_MAXLEN];
} kmem_cache_t;

typedef uint kmem_bufctl;

extern kmem_cache_t *sized_caches[SIZED_CACHE_COUNT];

void kmem_init();
kmem_cache_t *kmem_cache_create(const char *name,
								size_t size,
								uint flags);

void *kmem_cache_alloc(kmem_cache_t *cache);
void kmem_cache_destroy(kmem_cache_t *cache);
void kmem_cache_free(kmem_cache_t *cache, void *obj);
size_t kmem_cache_shrink(kmem_cache_t *cache);
size_t kmem_cache_reap();

