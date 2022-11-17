cmake_minimum_required(VERSION 3.16)

add_library(ulib STATIC usys.S ulib.c inet.c printf.c printfmt.c umalloc.c entry.c)

target_include_directories(ulib BEFORE
    PUBLIC .
    PUBLIC ../../include)

set_property(SOURCE usys.S PROPERTY LANGUAGE C)

#-mno-implicit-float is used to avoid xmm registers, which may cause GPF
target_compile_options(ulib BEFORE
    PRIVATE -fno-pic
    PRIVATE -static
    PRIVATE -fno-builtin
    PRIVATE -fno-strict-aliasing
    PRIVATE -O2
    PRIVATE -nostdinc
    PRIVATE -Wall
    PRIVATE -MD
    PRIVATE -ggdb
    PRIVATE -m32
    PRIVATE -fno-stack-protector
    PRIVATE -fno-omit-frame-pointer)


