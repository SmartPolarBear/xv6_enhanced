//
// Created by bear on 9/17/2022.
//

#include "types.h"
#include "stat.h"
#include "user.h"

#define EXIT_CODE 1204

void waitpid_test()
{
	int pid = fork();
	if (pid == 0)
	{
		exit(EXIT_CODE);
	}
	else
	{
		int exit_code = 0;
		int ret = waitpid(pid, &exit_code);
		if (ret != pid)
		{
			printf(1, "Wrong waitpid result %d vs %d\n", ret, pid);
			exit(-1);
		}

		if (exit_code != EXIT_CODE)
		{
			printf(1, "Wrong exit code result %d vs %d\n", exit_code, EXIT_CODE);
			exit(-1);
		}
	}
	printf(1, "waitpid test OK\n");
}

int main()
{
	waitpid_test();
	return 0;
}