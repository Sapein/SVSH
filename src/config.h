#ifndef SVSH_CONFIG
#include "conversions.h"
#define SVSH_CONFIG
#define EMPTY()
#define DEFER(x) x EMPTY()

/* 'FS' Configuration, all numbers are in Bytes, unless a conversion function is called */
#define FS_SIZE DEFER(SVSH_KIB_TO_B(10))
#define FS_FSLT_SIZE DEFER(SVSH_KIB_TO_B(1)) /* This sets the FSLT to 10% of the Filesystem's Size */
#define FS_BLOCK_SIZE 10
#endif
