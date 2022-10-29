//
// Created by bear on 10/29/2022.
//
#include "types.h"
#include "defs.h"
#include "date.h"

// time.c
extern uint32 sys_now(void);

unsigned long lwip_rand(void)
{
	uint32 time = sys_now();
	static unsigned int z1 = 12345, z2 = 12345, z3 = 12345, z4 = 12345;
	unsigned int b;
	b  = ((z1 << 6) ^ z1) >> 13;
	z1 = ((z1 & time) << 18) ^ b;
	b  = ((z2 << 2) ^ z2) >> 27;
	z2 = ((z2 & time) << 2) ^ b;
	b  = ((z3 << 13) ^ z3) >> 21;
	z3 = ((z3 & time) << 7) ^ b;
	b  = ((z4 << 3) ^ z4) >> 12;
	z4 = ((z4 & time) << 13) ^ b;
	return (z1 ^ z2 ^ z3 ^ z4);
}