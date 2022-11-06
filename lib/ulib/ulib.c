#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"
#include "socket.h"

char *
strcpy(char *s, const char *t)
{
	char *os;

	os = s;
	while ((*s++ = *t++) != 0);
	return os;
}

int
strcmp(const char *p, const char *q)
{
	while (*p && *p == *q)
		p++, q++;
	return (uchar)*p - (uchar)*q;
}

uint
strlen(const char *s)
{
	int n;

	for (n = 0; s[n]; n++);
	return n;
}

void *
memset(void *dst, int c, uint n)
{
	stosb(dst, c, n);
	return dst;
}

char *
strchr(const char *s, char c)
{
	for (; *s; s++)
		if (*s == c)
		{
			return (char *)s;
		}
	return 0;
}

char *
gets(char *buf, int max)
{
	int i, cc;
	char c;

	for (i = 0; i + 1 < max;)
	{
		cc = read(0, &c, 1);
		if (cc < 1)
		{
			break;
		}
		buf[i++] = c;
		if (c == '\n' || c == '\r')
		{
			break;
		}
	}
	buf[i] = '\0';
	return buf;
}

int
stat(const char *n, struct stat *st)
{
	int fd;
	int r;

	fd = open(n, O_RDONLY);
	if (fd < 0)
	{
		return -1;
	}
	r = fstat(fd, st);
	close(fd);
	return r;
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

void *
memmove(void *vdst, const void *vsrc, int n)
{
	char *dst;
	const char *src;

	dst = vdst;
	src = vsrc;
	while (n-- > 0)
		*dst++ = *src++;
	return vdst;
}

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
	return endian == __LITTLE_ENDIAN ? byteswap16(h) : h;
}

uint16_t
ntoh16(uint16_t n)
{
	if (!endian)
	{
		endian = byteorder();
	}
	return endian == __LITTLE_ENDIAN ? byteswap16(n) : n;
}

uint32_t
hton32(uint32_t h)
{
	if (!endian)
	{
		endian = byteorder();
	}
	return endian == __LITTLE_ENDIAN ? byteswap32(h) : h;
}

uint32_t
ntoh32(uint32_t n)
{
	if (!endian)
	{
		endian = byteorder();
	}
	return endian == __LITTLE_ENDIAN ? byteswap32(n) : n;
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
ip_addr_pton(const char *p, ip_addr_t *n)
{
	char *sp, *ep;
	int idx;
	long ret;

	sp = (char *)p;
	for (idx = 0; idx < 4; idx++)
	{
		ret = strtol(sp, &ep, 10);
		if (ret < 0 || ret > 255)
		{
			return -1;
		}
		if (ep == sp)
		{
			return -1;
		}
		if ((idx == 3 && *ep != '\0') || (idx != 3 && *ep != '.'))
		{
			return -1;
		}
		((uint8_t *)n)[idx] = ret;
		sp = ep + 1;
	}
	return 0;
}

