#include "types.h"
#include "user.h"
#include "signal.h"

extern int main(int argc, char **argv);

void terminate_signal(int signum)
{
	switch (signum)
	{
	case SIGFPE:
		printf(2, "Divided by 0\n");
		break;
	default:
		break;
	}
	exit(-1);
}

void _ulib_start(int argc, char **argv)
{
	// install signal handlers
	signal(SIGFPE, terminate_signal);

	int ret = main(argc, argv);
	exit(ret);
}