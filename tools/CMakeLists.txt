cmake_minimum_required(VERSION 3.16)

add_executable(mkfs mkfs.c)

target_compile_options(mkfs BEFORE
    PRIVATE -Wall)