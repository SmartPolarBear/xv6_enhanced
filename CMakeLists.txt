cmake_minimum_required(VERSION 3.16)

project(xv6-modern C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(Python COMPONENTS Interpreter)

if(!Python_FOUND)
    message(ERROR "A python interpreter with higher version than 3.9.0 is required")
else()
    message(STATUS "Use python executable ${Python_EXECUTABLE}")
endif()

# mkfs tool
add_executable(mkfs mkfs.c)

target_compile_options(mkfs BEFORE
    PRIVATE -Wall)

# user library
add_subdirectory(lib)

# user applications
add_subdirectory(user)

# bootblock
add_executable(bootblock_obj bootasm.S bootmain.c)

set_property(SOURCE bootasm.S PROPERTY LANGUAGE C)

target_compile_options(bootblock_obj BEFORE
    PRIVATE -fno-pic
    PRIVATE -m32
    PRIVATE -Os
    PRIVATE -nostdinc)

target_link_options(bootblock_obj BEFORE
    PRIVATE -Wl,-N
    PRIVATE -m32
    PRIVATE -nostdlib
    PRIVATE -static
    PRIVATE -Wl,--build-id=none
    PRIVATE -Wl,-T ${CMAKE_CURRENT_SOURCE_DIR}/bootblock.ld)

add_custom_command(TARGET bootblock_obj POST_BUILD
    COMMAND objdump -S $<TARGET_FILE:bootblock_obj> > ${CMAKE_BINARY_DIR}/bootblock.asm
    COMMAND objcopy -S -O binary -j .text $<TARGET_FILE:bootblock_obj> ${CMAKE_BINARY_DIR}/bootblock
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/sign.py ${CMAKE_BINARY_DIR}/bootblock
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# entryother
add_executable(entryother_obj entryother.S)

set_property(SOURCE entryother.S PROPERTY LANGUAGE C)

target_compile_options(entryother_obj BEFORE
    PRIVATE -fno-pic
    PRIVATE -m32
    PRIVATE -nostdinc)

target_link_options(entryother_obj BEFORE
    PRIVATE -N
    PRIVATE -m32
    PRIVATE -nostdlib
    PRIVATE -e start
    PRIVATE -static
    PRIVATE -Ttext 0x7000)

add_custom_command(TARGET entryother_obj POST_BUILD
    COMMAND objdump -S $<TARGET_FILE:entryother_obj> > ${CMAKE_BINARY_DIR}/entryother.asm
    COMMAND objcopy -S -O binary -j .text $<TARGET_FILE:entryother_obj> ${CMAKE_BINARY_DIR}/entryother
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# initcode
add_executable(initcode_obj initcode.S)

set_property(SOURCE initcode.S PROPERTY LANGUAGE C)

target_compile_options(initcode_obj BEFORE
    PRIVATE -fno-pic
    PRIVATE -m32
    PRIVATE -nostdinc)

target_link_options(initcode_obj BEFORE
    PRIVATE -N
    PRIVATE -m32
    PRIVATE -nostdlib
    PRIVATE -e start
    PRIVATE -static
    PRIVATE -Ttext 0)

add_custom_command(TARGET initcode_obj POST_BUILD
    COMMAND objdump -S $<TARGET_FILE:initcode_obj> > ${CMAKE_BINARY_DIR}/initcode.asm
    COMMAND objcopy -S -O binary -j .text $<TARGET_FILE:initcode_obj> ${CMAKE_BINARY_DIR}/initcode
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# kernel
add_executable(kernel
    bio.c
    console.c
    exec.c
    file.c
    fs.c
    ide.c
    ioapic.c
    kalloc.c
    kbd.c
    lapic.c
    log.c
    main.c
    mp.c
    picirq.c
    pipe.c
    proc.c
    sleeplock.c
    spinlock.c
    string.c
    syscall.c
    sysfile.c
    sysproc.c
    trap.c
    uart.c
    vm.c)

add_dependencies(kernel vectors_S)

target_sources(kernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/trapasm.S)
target_sources(kernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vectors.S)
target_sources(kernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/swtch.S)
target_sources(kernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/entry.S)

set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/trapasm.S PROPERTY LANGUAGE C)
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/vectors.S PROPERTY LANGUAGE C)
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/swtch.S PROPERTY LANGUAGE C)
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/entry.S PROPERTY LANGUAGE C)

target_compile_options(kernel BEFORE
    PRIVATE -fno-pic
    PRIVATE -static
    PRIVATE -fno-builtin
    PRIVATE -fno-strict-aliasing
    PRIVATE -O2
    PRIVATE -Wall
    PRIVATE -MD
    PRIVATE -ggdb
    PRIVATE -m32
    PRIVATE -Werror
    PRIVATE -fno-stack-protector
    PRIVATE -fno-omit-frame-pointer)

target_link_options(kernel BEFORE
    PRIVATE -m32
    PRIVATE -nostdlib
    PRIVATE -static
    PRIVATE -ffreestanding
    PRIVATE -nostartfiles
    PRIVATE -static
    PRIVATE -nostdlib
    PRIVATE -Wl,--build-id=none
    PRIVATE -T ${CMAKE_CURRENT_SOURCE_DIR}/kernel.ld)

target_link_options(kernel
    PRIVATE -Wl,-L ${CMAKE_BINARY_DIR} # to avoid extra strings in symbol names
    PRIVATE -Wl,--format=binary initcode entryother
    PRIVATE -Wl,--format=elf32-i386 # will be insert before the rest of the objects
)

add_custom_target(
    vectors_S ALL
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/vectors.py ${CMAKE_CURRENT_SOURCE_DIR}/vectors.S
    BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/vectors.S
    COMMENT "Generating vectors"
)

add_custom_command(TARGET kernel POST_BUILD
    COMMAND objdump -S $<TARGET_FILE:kernel> > ${CMAKE_BINARY_DIR}/kernel.asm
    COMMAND objdump -t $<TARGET_FILE:kernel> > ${CMAKE_BINARY_DIR}/kernel.sym
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(xv6_img ALL
        COMMAND dd if=/dev/zero of=${CMAKE_CURRENT_SOURCE_DIR}/xv6.img count=10000
        COMMAND dd if=${CMAKE_BINARY_DIR}/bootblock of=xv6.img conv=notrunc
        COMMAND dd if=$<TARGET_FILE:kernel> of=${CMAKE_CURRENT_SOURCE_DIR}/xv6.img seek=1 conv=notrunc
        DEPENDS kernel bootblock_obj
        BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/xv6.img
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating xv6.img ramdisk")

add_custom_target(fs_img ALL
    COMMAND $<TARGET_FILE:mkfs> ${CMAKE_CURRENT_SOURCE_DIR}/fs.img ${CMAKE_CURRENT_SOURCE_DIR}/README $<TARGET_FILE:_zombie> $<TARGET_FILE:_wc> $<TARGET_FILE:_usertests> $<TARGET_FILE:_stressfs> $<TARGET_FILE:_sh> $<TARGET_FILE:_cat> $<TARGET_FILE:_echo> $<TARGET_FILE:_forktest> $<TARGET_FILE:_grep> $<TARGET_FILE:_init> $<TARGET_FILE:_kill> $<TARGET_FILE:_ln> $<TARGET_FILE:_ls> $<TARGET_FILE:_mkdir> $<TARGET_FILE:_rm>
    DEPENDS mkfs _cat _echo _forktest _grep _init _kill _ln _ls _mkdir _rm _sh _stressfs _usertests _wc _zombie
    BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/fs.img
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating fs.img ramdisk")