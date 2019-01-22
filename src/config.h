#ifndef SVSH_CONFIG
#include "conversions.h"
#define SVSH_CONFIG
#define EMPTY()
#define DEFER(x) x EMPTY()

/* 'FS' Configuration */
#define FS_SIZE DEFER(SVSH_KIB_TO_B(10))
#define FS_FSLT_SIZE DEFER(SVSH_KIB_TO_B(1)) /* This sets the FSLT to 10% of the Filesystem's Size */
#endif
