#include "types.h"
#include "user.h"

extern int main(int argc, char** argv);

void _ulib_start()
{
    main(0, 0);
    exit();
}