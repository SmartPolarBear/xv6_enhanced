cmake_minimum_required(VERSION 3.16)

target_sources(kernel
        PRIVATE kalloc.c
        PRIVATE memory.c
        PRIVATE pmm.c
        PRIVATE slab.c
        PRIVATE vmm.c)