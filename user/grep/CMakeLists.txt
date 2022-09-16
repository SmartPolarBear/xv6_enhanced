cmake_minimum_required(VERSION 3.16)

add_executable(_grep grep.c)

add_dependencies(_grep ulib)

target_link_libraries(_grep ulib)

target_compile_options(_grep BEFORE
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

target_link_options(_grep BEFORE
    PRIVATE -m32
    PRIVATE -nostdlib
    PRIVATE -static
    PRIVATE -Wl,--build-id=none
    PRIVATE -T ${CMAKE_CURRENT_SOURCE_DIR}/../user.ld)


add_custom_command(TARGET _grep POST_BUILD
    COMMAND objdump -S $<TARGET_FILE:_grep> > ${CMAKE_CURRENT_BINARY_DIR}/grep.asm
    COMMAND objdump -t $<TARGET_FILE:_grep> > ${CMAKE_CURRENT_BINARY_DIR}/grep.sym
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

