//
// Created by bear on 10/17/2022.
//

#pragma once

#include "types.h"

enum gfp_flags
{
	GFP_KERNEL = 0x01,
	GFP_ATOMIC = 0x02,
	GFP_NOIO = 0x04,
	GFP_NOFS = 0x08,
	GFP_NOWAIT = 0x10,
	GFP_NOFAIL = 0x20,
	GFP_NORETRY = 0x40,
};

void *kmalloc(size_t size, gfp_t flags);
void kmfree(void *ptr);