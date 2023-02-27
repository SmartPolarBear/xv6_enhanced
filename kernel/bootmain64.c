
static int pos = 0;
volatile unsigned short *crt = (unsigned short *)0xb8000;

void puts(const char *s)
{
	while (*s)
	{
		crt[pos++] = (crt[pos] & 0xff00) | *s++;
		if ((pos / 80) >= 24)
		{
			pos = 0;
		}
	}
}

int boot64main()
{
	puts("fuck!");
	for (;;)
	{
	}
}