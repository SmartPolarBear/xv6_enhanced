//
// Created by bear on 11/7/2022.
//

#include "types.h"
#include "defs.h"
#include "netdb.h"

int sys_gethostbyname(void)
{
	char *name = NULL;
	if (argstr(0, &name) < 0)
	{
		return -1;
	}

	return (int)gethostbyname(name);
}

int sys_gethostbyaddr(void)
{
	return 0;
}