cmake_minimum_required(VERSION 3.22)

target_include_directories(kernel
        PRIVATE include)

target_sources(kernel PRIVATE port.c)