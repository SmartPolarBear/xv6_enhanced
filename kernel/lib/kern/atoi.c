//
// Created by bear on 10/20/2022.
//

#include <limits.h>

int isspace(int c)
{
	return (c == '\t' || c == '\n' ||
		c == '\v' || c == '\f' || c == '\r' || c == ' ' ? 1 : 0);
}

int isalpha(int c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int isdigit(int c)
{
	return c >= '0' && c <= '9';
}

int isupper(int c)
{
	return c >= 'A' && c <= 'Z';
}

int
atoi(const char *s)
{
	int n;

	n = 0;
	while ('0' <= *s && *s <= '9')
		n = n * 10 + *s++ - '0';
	return n;
}

long strtol(const char *str, char **endptr, int base)
{
	int c;
	long total;
	int sign;

	while (isspace((int)(unsigned char)*str))
		str++;

	c = (int)(unsigned char)*str++;
	sign = c;
	if (c == '-' || c == '+')
		c = (int)(unsigned char)*str++;

	total = 0;
	if (base == 0)
	{
		if (c == '0')
		{
			c = (int)(unsigned char)*str++;
			if (c == 'x' || c == 'X')
			{
				c = (int)(unsigned char)*str++;
				base = 16;
			}
			else
			{
				base = 8;
			}
		}
		else
		{
			base = 10;
		}
	}
	else if (base == 16)
	{
		if (c == '0')
		{
			c = (int)(unsigned char)*str++;
			if (c == 'x' || c == 'X')
			{
				c = (int)(unsigned char)*str++;
			}
		}
	}

	while (1)
	{
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		total = base * total + c;
		c = (int)(unsigned char)*str++;
	}

	if (endptr != 0)
		*endptr = (char *)(c ? str - 1 : str);

	if (sign == '-')
		return -total;
	else
		return total;
}