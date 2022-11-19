#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"
#include "inet.h"
#include "socket.h"

#define isascii(x) ((x >= 0x00) && (x <= 0x7f))
#define isprint(x) ((x >= 0x20) && (x <= 0x7e))

void
hexdump(void *data, size_t size)
{
	int offset, index;
	unsigned char *src;

	src = (unsigned char *)data;
	printf(1, "+------+-------------------------------------------------+------------------+\n");
	for (offset = 0; offset < size; offset += 16)
	{
		printf(1, "| ");
		if (offset <= 0x0fff)
		{
			printf(1, "0");
		}
		if (offset <= 0x00ff)
		{
			printf(1, "0");
		}
		if (offset <= 0x000f)
		{
			printf(1, "0");
		}
		printf(1, "%x | ", offset);
		for (index = 0; index < 16; index++)
		{
			if (offset + index < (int)size)
			{
				if (src[offset + index] <= 0x0f)
				{
					printf(1, "0");
				}
				printf(1, "%x ", 0xff & src[offset + index]);
			}
			else
			{
				printf(1, "   ");
			}
		}
		printf(1, "| ");
		for (index = 0; index < 16; index++)
		{
			if (offset + index < (int)size)
			{
				if (isascii(src[offset + index]) && isprint(src[offset + index]))
				{
					printf(1, "%c", src[offset + index]);
				}
				else
				{
					printf(1, ".");
				}
			}
			else
			{
				printf(1, " ");
			}
		}
		printf(1, " |\n");
	}
	printf(1, "+------+-------------------------------------------------+------------------+\n");
}

#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN 4321
#endif
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1234
#endif

static int endian;

static int
byteorder(void)
{
	uint32_t x = 0x00000001;
	return *(uint8_t *)&x ? __LITTLE_ENDIAN : __BIG_ENDIAN;
}

uint16_t
hton16(uint16_t h)
{
	if (!endian)
	{
		endian = byteorder();
	}
	return endian == __LITTLE_ENDIAN ? __builtin_bswap16(h) : h;
}

uint16_t
ntoh16(uint16_t n)
{
	if (!endian)
	{
		endian = byteorder();
	}
	return endian == __LITTLE_ENDIAN ? __builtin_bswap16(n) : n;
}

uint32_t
hton32(uint32_t h)
{
	if (!endian)
	{
		endian = byteorder();
	}
	return endian == __LITTLE_ENDIAN ? __builtin_bswap32(h) : h;
}

uint32_t
ntoh32(uint32_t n)
{
	if (!endian)
	{
		endian = byteorder();
	}
	return endian == __LITTLE_ENDIAN ? __builtin_bswap32(n) : n;
}

long
strtol(const char *s, char **endptr, int base)
{
	int neg = 0;
	long val = 0;

	// gobble initial whitespace
	while (*s == ' ' || *s == '\t')
		s++;

	// plus/minus sign
	if (*s == '+')
	{
		s++;
	}
	else if (*s == '-')
	{
		s++, neg = 1;
	}

	// hex or octal base prefix
	if ((base == 0 || base == 16) && (s[0] == '0' && s[1] == 'x'))
	{
		s += 2, base = 16;
	}
	else if (base == 0 && s[0] == '0')
	{
		s++, base = 8;
	}
	else if (base == 0)
	{
		base = 10;
	}

	// digits
	while (1)
	{
		int dig;

		if (*s >= '0' && *s <= '9')
		{
			dig = *s - '0';
		}
		else if (*s >= 'a' && *s <= 'z')
		{
			dig = *s - 'a' + 10;
		}
		else if (*s >= 'A' && *s <= 'Z')
		{
			dig = *s - 'A' + 10;
		}
		else
		{
			break;
		}
		if (dig >= base)
		{
			break;
		}
		s++, val = (val * base) + dig;
		// we don't properly detect overflow!
	}

	if (endptr)
	{
		*endptr = (char *)s;
	}
	return (neg ? -val : val);
}

int
inet_aton(const char *cp, struct in_addr *ap)
{
	int dots = 0;
	register uint32 acc = 0, addr = 0;

	do
	{
		register char cc = *cp;

		switch (cc)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			acc = acc * 10 + (cc - '0');
			break;

		case '.':
			if (++dots > 3)
			{
				return 0;
			}
			/* Fall through */

		case '\0':
			if (acc > 255)
			{
				return 0;
			}
			addr = addr << 8 | acc;
			acc = 0;
			break;

		default:
			return 0;
		}
	} while (*cp++);

	/* Normalize the address */
	if (dots < 3)
	{
		addr <<= 8 * (3 - dots);
	}

	/* Store it if requested */
	if (ap)
	{
		ap->s_addr = htonl(addr);
	}

	return 1;
}

static char buf[INET_ADDRSTRLEN];

char *inet_ntoa(struct in_addr in)
{
	uint8 *p = (uint8 *)&in.s_addr;
	memset(buf, 0, sizeof(buf));
	snprintf(buf, INET_ADDRSTRLEN, "%d.%d.%d.%d", p[3], p[2], p[1], p[0]);
	return buf;
}

in_addr_t inet_addr(const char *cp)
{
	struct in_addr val;

	if (inet_aton(cp, &val))
	{
		return val.s_addr;
	}

	return INADDR_NONE;
}