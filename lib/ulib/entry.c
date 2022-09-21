#include "types.h"
#include "user.h"
#include "signal.h"

extern int main(int argc, char **argv);

void terminate_signal(int signum)
{
	switch (signum)
	{
	case SIGALRM:
		printf(2, "SIGALRM received.\n");
		break;
	case SIGFPE:
		printf(2, "Floating point error.\n");
		break;
	case SIGINT:
		printf(2, "SIGINT received.\n");
		break;
	default:
		break;
	}
	exit(128 + signum);
}

void _ulib_start(int argc, char **argv)
{
	// install signal handlers
	signal(SIGABRT, terminate_signal);
	signal(SIGALRM, terminate_signal);
	signal(SIGFPE, terminate_signal);
	signal(SIGINT, terminate_signal);
	signal(SIGHUP, terminate_signal);
	signal(SIGKILL, terminate_signal);
	signal(SIGPIPE, terminate_signal);
	signal(SIGQUIT, terminate_signal);
	signal(SIGSEGV, terminate_signal);
	signal(SIGTERM, terminate_signal);
	signal(SIGUSR1, terminate_signal);
	signal(SIGUSR2, terminate_signal);

	int ret = main(argc, argv);
	exit(ret);
}