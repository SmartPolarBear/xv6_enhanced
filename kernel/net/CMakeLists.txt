cmake_minimum_required(VERSION 3.16)

add_subdirectory(drivers)

target_sources(kernel
        PRIVATE net.c
        PRIVATE netdb.c
        PRIVATE socket.c
        PRIVATE syssocket.c
        PRIVATE sysnetdb.c)