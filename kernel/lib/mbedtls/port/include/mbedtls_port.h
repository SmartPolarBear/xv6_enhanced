//
// Created by bear on 11/29/2022.
//

#pragma once

void *mbedtls_calloc_impl(unsigned int, unsigned int);
void mbedtls_free_impl(void *mem);
unsigned int mbedtls_time_impl(unsigned int *timer);

#define MBEDTLS_PLATFORM_STD_CALLOC        mbedtls_calloc_impl /**< Default allocator to use, can be undefined */
#define MBEDTLS_PLATFORM_STD_FREE            mbedtls_free_impl /**< Default free to use, can be undefined */
//#define MBEDTLS_PLATFORM_STD_EXIT            exit /**< Default exit to use, can be undefined */
#define MBEDTLS_PLATFORM_STD_TIME            mbedtls_time_impl /**< Default time to use, can be undefined. MBEDTLS_HAVE_TIME must be enabled */
//#define MBEDTLS_PLATFORM_STD_FPRINTF      fprintf /**< Default fprintf to use, can be undefined */
#define MBEDTLS_PLATFORM_STD_PRINTF        cprintf /**< Default printf to use, can be undefined */
/* Note: your snprintf must correctly zero-terminate the buffer! */
#define MBEDTLS_PLATFORM_STD_SNPRINTF    snprintf /**< Default snprintf to use, can be undefined */
#define MBEDTLS_PLATFORM_STD_EXIT_SUCCESS       0 /**< Default exit value to use, can be undefined */
#define MBEDTLS_PLATFORM_STD_EXIT_FAILURE       1 /**< Default exit value to use, can be undefined */
//#define MBEDTLS_PLATFORM_STD_NV_SEED_READ   mbedtls_platform_std_nv_seed_read /**< Default nv_seed_read function to use, can be undefined */
//#define MBEDTLS_PLATFORM_STD_NV_SEED_WRITE  mbedtls_platform_std_nv_seed_write /**< Default nv_seed_write function to use, can be undefined */
//#define MBEDTLS_PLATFORM_STD_NV_SEED_FILE  "seedfile" /**< Seed file to read/write with default implementation */

#define MBEDTLS_PLATFORM_TIME_TYPE_MACRO unsigned int


