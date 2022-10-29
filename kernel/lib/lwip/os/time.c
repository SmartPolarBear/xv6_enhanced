//
// Created by bear on 10/29/2022.
//
#include "types.h"
#include "defs.h"
#include "date.h"

uint32
sys_now(void)
{
	rtcdate_t date;
	cmostime(&date);
	uint32 ret = unixime_in_seconds(&date) * 1000u;
	return ret;
}