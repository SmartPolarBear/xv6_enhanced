//
// Created by bear on 10/21/2022.
//
#include "types.h"
#include "date.h"
#include "defs.h"

#define SEC_PER_MIN         60
#define SEC_PER_HOUR        3600
#define SEC_PER_DAY         86400
#define MOS_PER_YEAR        12
#define EPOCH_YEAR          1970
#define IS_LEAP_YEAR(year)  ( (((year)%4 == 0) && ((year)%100 != 0)) || ((year)%400 == 0) )

static int days_per_month[2][MOS_PER_YEAR] = {
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

static int days_per_year[2] = {
	365, 366
};

uint32 unixtime_in_seconds(const rtcdate_t *date)
{
	uint32 ts = 0;

	//  Add up the seconds from all prev years, up until this year.
	uint8 years = 0;
	uint8 leap_years = 0;
	for (uint16 y_k = EPOCH_YEAR; y_k < date->year; y_k++)
	{
		if (IS_LEAP_YEAR(y_k))
		{
			leap_years++;
		}
		else
		{
			years++;
		}
	}
	ts += ((years * days_per_year[0]) + (leap_years * days_per_year[1])) * SEC_PER_DAY;

	//  Add up the seconds from all prev days this year, up until today.
	uint8 year_index = (IS_LEAP_YEAR(date->year)) ? 1 : 0;
	for (uint8 mo_k = 0; mo_k < (date->month - 1); mo_k++)
	{ //  days from previous months this year
		ts += days_per_month[year_index][mo_k] * SEC_PER_DAY;
	}
	ts += (date->day - 1) * SEC_PER_DAY; // days from this month

	//  Calculate seconds elapsed just today.
	ts += date->hour * SEC_PER_HOUR;
	ts += date->minute * SEC_PER_MIN;
	ts += date->second;

	return ts;
}
