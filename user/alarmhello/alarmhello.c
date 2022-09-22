#include "types.h"
#include "stat.h"
#include "user.h"
#include "signal.h"

int i = 0;
void alarm_handle(int signum)
{
	if (i >= 10)
	{
		alarm(-1);
	}
	i++;
	printf(1, "hello, %d\n", i);
	alarm(200 * i);
}

int main(int argc, char *argv[])
{
	printf(1, "start\n");
	signal(SIGALRM, alarm_handle);
	alarm(50);
	while (i <= 10);
	printf(1, "end\n");
	return 0;
}
