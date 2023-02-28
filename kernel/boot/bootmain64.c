#include "types.h"

enum colors
{
	COLOR_DEFAULT = 0x07,
	COLOR_ERROR = 0x1f
};

// bootcon64.c
extern void boot_puts(const char *s, uint8 color);
extern void set_cursor(int pos);

// bootdisk64.c
void load_kernel();

int boot64main()
{
	// "STARTING XV6" = 12
	set_cursor(12);
	boot_puts("...", COLOR_DEFAULT);
	set_cursor(80);



	load_kernel();

	// should not reach here normally.
	boot_puts("KERNEL EXITS UNEXPECTEDLY.", COLOR_ERROR);

	for (;;)
	{
	}
}