#pragma once

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long uint64;

typedef uint pde_t;
typedef uint gfp_t;

#ifdef __KERNEL__
typedef uint32 size_t;
typedef uint32 uintptr_t;
#endif

// Efficient min and max operations
#define MIN(_a, _b)                        \
({                                \
    typeof(_a) __a = (_a);                    \
    typeof(_b) __b = (_b);                    \
    __a <= __b ? __a : __b;                    \
})
#define MAX(_a, _b)                        \
({                                \
    typeof(_a) __a = (_a);                    \
    typeof(_b) __b = (_b);                    \
    __a >= __b ? __a : __b;                    \
})

// Rounding operations (efficient when n is a power of 2)
// Round down to the nearest multiple of n
#define ROUNDDOWN(a, n)                        \
({                                \
    uint32 __a = (uint32) (a);                \
    (typeof(a)) (__a - __a % (n));                \
})

// Round up to the nearest multiple of n
#define ROUNDUP(a, n)                        \
({                                \
    uint32 __n = (uint32) (n);                \
    (typeof(a)) (ROUNDDOWN((uint32) (a) + __n - 1, __n));    \
})

typedef void (*sighandler_t)(int);

typedef struct list_head
{
	struct list_head *prev, *next;
} list_head_t;