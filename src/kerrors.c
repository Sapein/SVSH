#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "kerrors.h"

_Noreturn void SVSH_Kernel_Panic(char *fmt, ...){
    va_list data;
    va_start(data, fmt);
    vfprintf(stderr, fmt, data);
    va_end(data);
    exit(0);
}
