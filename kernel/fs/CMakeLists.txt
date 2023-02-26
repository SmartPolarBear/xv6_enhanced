cmake_minimum_required(VERSION 3.16)

target_sources(kernel
        PRIVATE bio.c
        PRIVATE file.c
        PRIVATE fs.c
        PRIVATE log.c
        PRIVATE vfs.c
        PRIVATE sysfile.c)