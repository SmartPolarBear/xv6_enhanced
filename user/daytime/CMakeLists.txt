cmake_minimum_required(VERSION 3.16)

add_executable(_daytime daytime.c)

add_dependencies(_daytime ulib)

target_link_libraries(_daytime ulib)

target_compile_options(_daytime BEFORE
        PRIVATE -fno-pic
        PRIVATE -static
        PRIVATE -fno-builtin
        PRIVATE -fno-strict-aliasing
        PRIVATE -O2
        PRIVATE -Wall
        PRIVATE -MD
        PRIVATE -ggdb
        PRIVATE -m32
        PRIVATE -fno-stack-protector
        PRIVATE -fno-omit-frame-pointer)

target_link_options(_daytime BEFORE
        PRIVATE -m32
        PRIVATE -nostdlib
        PRIVATE -static
        PRIVATE -Wl,--build-id=none
        PRIVATE -T ${CMAKE_CURRENT_SOURCE_DIR}/../user.ld)


add_custom_command(TARGET _daytime POST_BUILD
        COMMAND objdump -S $<TARGET_FILE:_daytime> > ${CMAKE_CURRENT_BINARY_DIR}/daytime.asm
        COMMAND objdump -t $<TARGET_FILE:_daytime> > ${CMAKE_CURRENT_BINARY_DIR}/daytime.sym
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )

