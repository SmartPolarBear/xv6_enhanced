//
// Created by bear on 10/17/2022.
//

#pragma once

#include "types.h"

enum gfp_flags
{
	GFP_DMA = 0x1
};

void *kmalloc(size_t size, gfp_t flags);
void kfree(void *ptr);