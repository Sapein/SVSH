#ifndef SVSH_KERRORS
#define SVSH_KERRORS
#include <stdio.h>
#define Panic() SVSH_Kernel_Panic("%s() has caused an error. It called in %s at %d\n", __func__, __FILE__, __LINE__)
#define KVPanic(x, fmt, ...) SVSH_Kernel_Panic("%s: " fmt, (x), __VA_ARGS__)
#define KPanic(x, msg) SVSH_Kernel_Panic("%s: %s", (x), (msg))
#define KVLog(x, fmt, ...) fprintf(stderr, "%s: " fmt, (x), __VA_ARGS__)
#define KLog(x, msg) fprintf(stderr, "%s: %s", (x), (msg))

_Noreturn void SVSH_Kernel_Panic(char *fmt, ...);

#endif
