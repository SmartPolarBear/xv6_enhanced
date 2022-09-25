#pragma once

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef uint pde_t;

typedef void (*sighandler_t)(int);

typedef struct list_head
{
	struct list_head *prev, *next;
} list_head_t;