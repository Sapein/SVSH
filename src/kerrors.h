#ifndef SVSH_KERRORS
#define SVSH_KERRORS
#define Panic() SVSH_Kernel_Panic("%s() has caused an error. It called in %s at %d\n", __func__, __FILE__, __LINE__)

_Noreturn void SVSH_Kernel_Panic(char *fmt, ...);

#endif
