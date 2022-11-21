cmake_minimum_required(VERSION 3.16)

# bootblock
add_executable(bootblock_obj bootasm.S bootmain.c)

target_include_directories(bootblock_obj PRIVATE ../include)

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
        COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../tools/sign.py ${CMAKE_BINARY_DIR}/bootblock
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )

# entryother 
add_executable(entryother_obj entryother.S)

target_include_directories(entryother_obj PRIVATE ../include)

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
        COMMAND objdump -S $<TARGET_FILE:entryother_obj> > ${CMAKE_CURRENT_BINARY_DIR}/entryother.asm
        COMMAND objcopy -S -O binary -j .text $<TARGET_FILE:entryother_obj> ${CMAKE_CURRENT_BINARY_DIR}/entryother
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )

# initcode
add_executable(initcode_obj initcode.S)

target_include_directories(initcode_obj BEFORE PRIVATE ../include)

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
        COMMAND objdump -S $<TARGET_FILE:initcode_obj> > ${CMAKE_CURRENT_BINARY_DIR}/initcode.asm
        COMMAND objcopy -S -O binary -j .text $<TARGET_FILE:initcode_obj> ${CMAKE_CURRENT_BINARY_DIR}/initcode
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )

# kernel
add_executable(kernel
        atoi.c
        bio.c
        console.c
        debug.c
        date.c
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
        memory.c
        pci.c
        picirq.c
        pipe.c
        proc.c
        pmm.c
        sleeplock.c
        spinlock.c
        string.c
        slab.c
        syscall.c
        sysfile.c
        sysproc.c
        trap.c
        uart.c
        vfs.c
        vmm.c)

add_subdirectory(lib)
add_subdirectory(net)

target_compile_definitions(kernel
        PRIVATE -D__KERNEL__)

add_dependencies(kernel vectors_S initcode_obj entryother_obj bootblock_obj)

target_include_directories(kernel PRIVATE ../include)

target_sources(kernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/trapasm.S)
target_sources(kernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vectors.S)
target_sources(kernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/swtch.S)
target_sources(kernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/entry.S)
target_sources(kernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/signal.S)

set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/trapasm.S PROPERTY LANGUAGE C)
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/vectors.S PROPERTY LANGUAGE C)
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/swtch.S PROPERTY LANGUAGE C)
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/entry.S PROPERTY LANGUAGE C)
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/signal.S PROPERTY LANGUAGE C)

#target_link_libraries(kernel PRIVATE lwipcore)

target_compile_options(kernel BEFORE
        PRIVATE -U__USE_EXTERN_INLINES
        PRIVATE -fno-pic
        PRIVATE -static
        PRIVATE -fno-builtin
        PRIVATE -fno-strict-aliasing
        PRIVATE -O0
        PRIVATE -Wall
        PRIVATE -MD
        PRIVATE -ggdb
        PRIVATE -m32
        PRIVATE -g
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
        PRIVATE -Wl,-L ${CMAKE_CURRENT_BINARY_DIR} # to avoid extra strings in symbol names
        PRIVATE -Wl,--format=binary initcode entryother
        PRIVATE -Wl,--format=elf32-i386 # will be insert before the rest of the objects
        )

add_custom_target(
        vectors_S ALL
        COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../tools/vectors.py ${CMAKE_CURRENT_SOURCE_DIR}/vectors.S
        BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/vectors.S
        COMMENT "Generating vectors"
)

add_custom_command(TARGET kernel POST_BUILD
        COMMAND objdump -S $<TARGET_FILE:kernel> > ${CMAKE_BINARY_DIR}/kernel.asm
        COMMAND objdump -t $<TARGET_FILE:kernel> > ${CMAKE_BINARY_DIR}/kernel.sym
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )