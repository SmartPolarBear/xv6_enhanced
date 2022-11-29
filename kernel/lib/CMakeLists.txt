cmake_minimum_required(VERSION 3.22)

add_subdirectory(lwip)

set(USE_STATIC_MBEDTLS_LIBRARY ON)
add_subdirectory(mbedtls)