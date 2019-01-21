#ifndef SVSH_CONFIG
#include "fs/files.h"
#define SVSH_CONFIG
#define EMPTY()
#define DEFER(x) x EMPTY()

/* 'FS' Configuration */
#define FS_SIZE DEFER(SVSH_KIB_TO_B(10))
#endif
