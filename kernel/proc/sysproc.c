#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int errnos[NCPU];

void seterror(int err)
{
	pushcli();
	errnos[mycpu()->apicid] = err;
	popcli();
}

int
sys_fork(void)
{
	return fork();
}

int
sys_exit(void)
{
	int code = 0;

	if (argint(0, &code) < 0)
	{
		return -1;
	}

	exit(code);
	return 0;  // not reached
}

int
sys_wait(void)
{
	char *status = NULL;
	if (argptr(0, &status, sizeof(int)))
	{
		return -1;
	}

	return wait((int *)status);
}

int
sys_waitpid(void)
{
	int pid = -1;
	if (argint(0, &pid) < 0)
	{
		return -1;
	}

	char *status = NULL;
	if (argptr(1, &status, sizeof(int)))
	{
		return -1;
	}

	return waitpid(pid, (int *)status);
}

int
sys_kill(void)
{
	int pid = 0;

	if (argint(0, &pid) < 0)
	{
		return -1;
	}

	int signal = 0;
	if (argint(1, &signal) < 0)
	{
		return -1;
	}

	if (signal == SIGKILL) // SIGKILL should not be modified
	{
		return killproc(pid);
	}
	else
	{
		return signal_deliver(pid, signal);
	}
}

int
sys_getpid(void)
{
	return myproc()->pid;
}

int
sys_sbrk(void)
{
	int addr;
	int n;

	if (argint(0, &n) < 0)
	{
		return -1;
	}
	addr = myproc()->sz;
	if (growproc(n) < 0)
	{
		return -1;
	}
	return addr;
}

int
sys_sleep(void)
{
	int n;
	uint ticks0;

	if (argint(0, &n) < 0)
	{
		return -1;
	}
	acquire(&tickslock);
	ticks0 = ticks;
	while (ticks - ticks0 < n)
	{
		if (myproc()->killed)
		{
			release(&tickslock);
			return -1;
		}
		sleep(&ticks, &tickslock);
	}
	release(&tickslock);
	return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
	uint xticks;

	acquire(&tickslock);
	xticks = ticks;
	release(&tickslock);
	return xticks;
}

int sys_signal(void)
{
	int signum = 0;
	if (argint(0, &signum) < 0)
	{
		return -1;
	}

	int handler = 0;
	if (argint(1, &handler) < 0)
	{
		return -1;
	}

	return (int)signal(signum, (sighandler_t)handler);
}

int sys_sigreturn(void)
{
	signal_return();
	return 0;
}

int sys_fgproc(void)
{
	return myproc()->pid;
}

int sys_alarm(void)
{
	int ticks = 0;
	if (argint(0, &ticks) < 0)
	{
		return -1;
	}

	myproc()->alarm_interval = ticks;
	myproc()->alarm_ticks = ticks;

	if (myproc()->alarm_interval > 0)
	{
		return myproc()->alarm_ticks;
	}
	return 0;
}

int sys_error()
{
	pushcli();
	int err = errnos[mycpu()->apicid];
	popcli();
	return err;
}