cmake_minimum_required(VERSION 3.16)

add_executable(_div0 div0.c)

add_dependencies(_div0 ulib)

target_link_libraries(_div0 ulib)

target_compile_options(_div0 BEFORE
        PRIVATE -fno-pic
        PRIVATE -static
        PRIVATE -fno-builtin
        PRIVATE -fno-strict-aliasing
        PRIVATE -O0
        PRIVATE -Wall
        PRIVATE -MD
        PRIVATE -ggdb
        PRIVATE -m32
        PRIVATE -fno-stack-protector
        PRIVATE -fno-omit-frame-pointer)

target_link_options(_div0 BEFORE
        PRIVATE -m32
        PRIVATE -nostdlib
        PRIVATE -static
        PRIVATE -Wl,--build-id=none
        PRIVATE -T ${CMAKE_CURRENT_SOURCE_DIR}/../user.ld)


add_custom_command(TARGET _div0 POST_BUILD
        COMMAND objdump -S $<TARGET_FILE:_div0> > ${CMAKE_CURRENT_BINARY_DIR}/div0.asm
        COMMAND objdump -t $<TARGET_FILE:_div0> > ${CMAKE_CURRENT_BINARY_DIR}/div0.sym
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )

