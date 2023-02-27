#include <types.h>

enum colors
{
	COLOR_DEFAULT = 0x07,
	COLOR_ERROR = 0x1f
};

static volatile uint16 *cga = (uint16 *)0xb8000;

static void boot_putc(const char c, int color)
{
	static int pos = 3;
	cga[0] = (c | 0x0700);
}

void boot_cls()
{

}

int boot64main()
{
	cga[3] = 0x0769;
	cga[4] = 0x4807;
	boot_putc('F', COLOR_DEFAULT);
	for (;;)
	{
	}
}