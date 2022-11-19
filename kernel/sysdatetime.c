//
// Created by bear on 11/19/2022.
//

#include "types.h"
#include "date.h"
#include "defs.h"

int sys_time(void)
{
	int *tloc;
	if (argptr(0, (void *)&tloc, sizeof(tloc)) < 0)
	{
		return -1;
	}

	rtcdate_t date;
	cmostime(&date);

	uint32 ret = unixime_in_seconds(&date);
	if (tloc)
	{
		*tloc = ret;
	}
	return ret;
}
