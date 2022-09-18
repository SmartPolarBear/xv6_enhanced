#include "types.h"
#include "user.h"

extern int main(int argc, char** argv);

void _ulib_start(int argc, char** argv)
{
	// install signal handlers

    int ret = main(argc, argv);
    exit(ret);
}