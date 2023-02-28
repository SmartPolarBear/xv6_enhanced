//
// Created by bear on 2/27/2023.
//
#include "types.h"
#include "x86.h"

#define CRTPORT 0x3d4
#define WIDTH 80
#define HEIGHT 25

static inline int get_cursor()
{
	int pos = 0;
	outb(CRTPORT, 14);
	pos = inb(CRTPORT + 1) << 8;
	outb(CRTPORT, 15);
	pos |= inb(CRTPORT + 1);
	return pos;
}

void set_cursor(int pos)
{
	outb(CRTPORT, 14);
	outb(CRTPORT + 1, pos >> 8);
	outb(CRTPORT, 15);
	outb(CRTPORT + 1, pos);
}

void boot_puts(const char *s, uint8 color)
{
	int pos = get_cursor();

	volatile uint16 *const cga = (uint16 *)0xb8000;
	while (*s)
	{
		cga[pos++] = (uint16)(*s++) | (uint16)color << 8;
	}

	set_cursor(pos);
}

