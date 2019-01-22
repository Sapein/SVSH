#include <stdio.h>
#include <stdlib.h>
#include "fs/files.h"
#include "sh/shell.h"
#include "config.h"

int main(void){
    SVSH_FS_Init(FS_SIZE);
    SVSH_SH_Init();

    SVSH_SH_Shutdown();
    SVSH_FS_HALT();
    return 0;
}
