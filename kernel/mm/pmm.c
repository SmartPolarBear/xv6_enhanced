//
// Created by bear on 11/4/2022.
//
#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "slab.h"

struct e820map_item e820_memmap[E820MAX] = {0};
size_t e820_memmap_len = 0;

void pmminit(void)
{

}