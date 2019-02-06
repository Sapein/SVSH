#ifndef SVSH_KERRORS
#define SVSH_KERRORS
#include <stdio.h>
#define Panic() SVSH_Kernel_Panic("%s() has caused an error. It called in %s at %d\n", __func__, __FILE__, __LINE__)
#define KPanic(x, msg) SVSH_Kernel_Panic("%s: %s", (x), (msg))
#define KERROR

_Noreturn void SVSH_Kernel_Panic(char *fmt, ...);

#endif
