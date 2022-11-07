cmake_minimum_required(VERSION 3.16)


target_sources(kernel
        PRIVATE e1000_nic.c
        PRIVATE virtio_nic.c)