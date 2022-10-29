#define LWIP_PLATFORM_DIAG(x) do { printf x; } while(0)
#define LWIP_PLATFORM_ASSERT(x) panic(x)

#define LWIP_NO_CTYPE_H 1
#define LWIP_NO_UNISTD_H 1

/* xv6 supporst a limited set of format specifiers */
#define LWIP_NO_INTTYPES_H 1
#define X8_F "x"
#define S16_F "d"
#define U16_F "u"
#define X16_F "x"
#define S32_F "d"
#define U32_F "u"
#define X32_F "x"
#define SZT_F "d"

typedef signed int ssize_t;

/* horrible rand */
#define LWIP_RAND lwip_rand

void printf(char *, ...);
void panic(char *) __attribute__((noreturn));
unsigned long lwip_rand(void);

