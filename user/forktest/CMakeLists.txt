cmake_minimum_required(VERSION 3.16)

add_executable(_forktest forktest.c)

add_dependencies(_forktest ulib)

target_link_libraries(_forktest ulib)

target_compile_options(_forktest BEFORE
    PRIVATE -fno-pic
    PRIVATE -static
    PRIVATE -fno-builtin
    PRIVATE -fno-strict-aliasing
    PRIVATE -Os
    PRIVATE -Wall
    PRIVATE -MD
    PRIVATE -ggdb
    PRIVATE -m32
    PRIVATE -Werror
    PRIVATE -fno-stack-protector
    PRIVATE -fno-omit-frame-pointer)

target_link_options(_forktest BEFORE
    PRIVATE -m32
    PRIVATE -nostdlib
    PRIVATE -static
    PRIVATE -Wl,--build-id=none
    PRIVATE -T ${CMAKE_CURRENT_SOURCE_DIR}/../user.ld)


add_custom_command(TARGET _forktest POST_BUILD
    COMMAND objdump -S $<TARGET_FILE:_forktest> > ${CMAKE_CURRENT_BINARY_DIR}/forktest.asm
    COMMAND objdump -t $<TARGET_FILE:_forktest> > ${CMAKE_CURRENT_BINARY_DIR}/forktest.sym
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

