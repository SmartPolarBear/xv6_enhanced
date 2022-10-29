#pragma once
#include "types.h"

typedef enum file_type
{
	FD_NONE = 1,
	FD_PIPE,
	FD_INODE,
	FD_DEVICE,
	FD_SOCKET,
} file_type_t;

typedef struct file
{
	file_type_t type;
	int ref; // reference count
	char readable;
	char writable;

	union
	{
		struct pipe *pipe;
		struct inode *ip;
		struct socket *socket;
	};
	uint off;
}file_t;

// in-memory copy of an inode
struct inode
{
	uint dev;           // Device number
	uint inum;          // Inode number
	int ref;            // Reference count
	struct sleeplock lock; // protects everything below here
	int valid;          // inode has been read from disk?

	short type;         // copy of disk inode
	short major;
	short minor;
	short nlink;
	uint size;
	uint addrs[NDIRECT + 1];
};

// table mapping major device number to
// device functions
struct devsw
{
	int (*read)(struct inode *, char *, int);
	int (*write)(struct inode *, char *, int);
};

extern struct devsw devsw[];

#define CONSOLE 1
