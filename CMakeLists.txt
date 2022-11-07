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

# tools
add_subdirectory(tools)

# user library
add_subdirectory(lib)

# user applications
add_subdirectory(user)

# kernel
add_subdirectory(kernel)

add_custom_target(xv6_img ALL
        COMMAND dd if=/dev/zero of=${CMAKE_CURRENT_SOURCE_DIR}/xv6.img count=10000
        COMMAND dd if=${CMAKE_BINARY_DIR}/bootblock of=xv6.img conv=notrunc
        COMMAND dd if=$<TARGET_FILE:kernel> of=${CMAKE_CURRENT_SOURCE_DIR}/xv6.img seek=1 conv=notrunc
        DEPENDS kernel bootblock_obj
        BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/xv6.img
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating xv6.img ramdisk")

add_custom_target(fs_img ALL
    COMMAND $<TARGET_FILE:mkfs> ${CMAKE_CURRENT_SOURCE_DIR}/fs.img ${CMAKE_CURRENT_SOURCE_DIR}/README
        $<TARGET_FILE:_alarmhello>
        $<TARGET_FILE:_div0>
        $<TARGET_FILE:_waittest>
        $<TARGET_FILE:_zombie>
        $<TARGET_FILE:_wc>
        $<TARGET_FILE:_usertests>
        $<TARGET_FILE:_stressfs>
        $<TARGET_FILE:_sh>
        $<TARGET_FILE:_cat>
        $<TARGET_FILE:_echo>
        $<TARGET_FILE:_forktest>
        $<TARGET_FILE:_grep>
        $<TARGET_FILE:_init>
        $<TARGET_FILE:_kill>
        $<TARGET_FILE:_ln>
        $<TARGET_FILE:_ls>
        $<TARGET_FILE:_mkdir>
        $<TARGET_FILE:_rm>
        $<TARGET_FILE:_tcpecho>
        $<TARGET_FILE:_udpecho>
        $<TARGET_FILE:_httpget>
        $<TARGET_FILE:_ping>
        $<TARGET_FILE:_traceroute>
        $<TARGET_FILE:_nslookup>
    DEPENDS mkfs _cat _echo _forktest _grep _init _kill _ln _ls _mkdir _rm _sh _stressfs _tcpecho _daytime _usertests _wc _zombie _udpecho _httpget _ping _traceroute _nslookup _waittest _div0 _alarmhello
    BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/fs.img
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating fs.img ramdisk")

add_custom_target(disk_images ALL
        DEPENDS fs_img xv6_img
        COMMENT "Generating all images including xv6.img and fs.img")