//
// Created by bear on 10/17/2022.
//

#include "types.h"
#include "console.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

// in console.c
extern consolectl_t cons;
extern int panicked;

void
panic(char *s)
{
	int i;
	uint pcs[10];

	cli();
	cons.locking = 0;
	// use lapiccpunum so that we can call panic from mycpu()
	cprintf("lapicid %d: panic: ", lapicid());
	cprintf(s);
	cprintf("\n");
	getcallerpcs(&s, pcs);
	for (i = 0; i < 10; i++)
		cprintf(" %p", pcs[i]);
	panicked = 1; // freeze other CPU
	for (;;);
}