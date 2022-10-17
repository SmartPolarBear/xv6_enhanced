#pragma once

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef uint pde_t;

#ifdef __KERNEL__
typedef uint size_t;
#endif

typedef void (*sighandler_t)(int);

typedef struct list_head
{
	struct list_head *prev, *next;
} list_head_t;