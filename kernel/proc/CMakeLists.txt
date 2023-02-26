cmake_minimum_required(VERSION 3.16)

target_sources(kernel
        PRIVATE exec.c
        PRIVATE door.c
        PRIVATE pipe.c
        PRIVATE proc.c
        PRIVATE sysproc.c)