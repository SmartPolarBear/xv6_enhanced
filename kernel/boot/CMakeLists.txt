cmake_minimum_required(VERSION 3.16)

add_executable(boot64 boot64.S bootmain64.c bootcon64.c bootdisk64.c)

target_include_directories(boot64 PRIVATE ../../include)

set_property(SOURCE boot64.S PROPERTY LANGUAGE C)

target_compile_definitions(boot64
        PRIVATE -D__KERNEL__)

target_compile_options(boot64 BEFORE
        PRIVATE -U__USE_EXTERN_INLINES
        PRIVATE -fno-pic
        PRIVATE -static
        PRIVATE -fno-builtin
        PRIVATE -fno-strict-aliasing
        PRIVATE -O0
        PRIVATE -Wall
        PRIVATE -MD
        PRIVATE -ggdb
        PRIVATE -m64
        PRIVATE -march=x86-64
        PRIVATE -g
        PRIVATE -mno-mmx
        PRIVATE -mno-sse
        PRIVATE -mcmodel=kernel
        PRIVATE -mtls-direct-seg-refs
        PRIVATE -mno-red-zone
        PRIVATE -mfpmath=387
        PRIVATE -fno-pie
        PRIVATE -fno-stack-protector
        PRIVATE -fno-omit-frame-pointer)

target_link_options(boot64 BEFORE
        PRIVATE -Wl,-m elf_x86_64
        PRIVATE -nostdlib
        PRIVATE -static
        PRIVATE -ffreestanding
        PRIVATE -nostartfiles
        PRIVATE -nodefaultlibs
        PRIVATE -Wl,--build-id=none
        PRIVATE -T ${CMAKE_CURRENT_SOURCE_DIR}/boot64.ld)

add_custom_command(TARGET boot64 POST_BUILD
        COMMAND objdump -S $<TARGET_FILE:boot64> > ${CMAKE_BINARY_DIR}/boot64.asm
        COMMAND objdump -t $<TARGET_FILE:boot64> > ${CMAKE_BINARY_DIR}/boot64.sym
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )