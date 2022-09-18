#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
	int x = 5;
	int y = 0;
	int a = y / x;
	int b = x / y;
	printf(1, "%d\n", a);
	printf(1, "%d\n", b);
	return 0;
}
