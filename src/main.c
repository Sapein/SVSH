#include <stdio.h>
#include <stdlib.h>
#include "fs/files.h"
#include "config.h"

int main(void){
    SVSH_FS_Init(FS_SIZE);
    SVSH_FS_HALT();
    return 0;
}
