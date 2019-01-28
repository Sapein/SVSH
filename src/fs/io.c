#include "io.h"
#include "file.h"

struct SVSH_FILE{
    struct AFile *original;
    uint8_t meta_blocks; /* Last two bits determine if either block is a meta block (1 = Meta, 0 = Not ) */

    uint8_t *block_1;
    uint8_t *block_2;

    /* Mode is setup as follows:
     * 00 - Write (00 = No Write ; 10 = Write Set, not permitted ; 11 = Write set and permitted ) 
     * 00 - Read  (00 = No read  ; 10 = Read set, not permitted  ; 11 = Read set and permitted  )
     * 00 - eXec  (00 = No eXec  ; 10 = eXec set, not permitted  ; 11 = eXec set and permitted  )
     * 00 - Open  (See AFile documentation)
     */
    uint8_t mode;
};

SVSH_FILE *SVSH_fopen(const char path[], const char mode[]){

}

int SVSH_fclose(SVSH_FILE *file);

size_t SVSH_fwrite(const void *input, size_t size, size_t nmemb, SVSH_FILE *stream);
size_t SVSH_fread(void *output, size_t size, size_t nmemb, SVSH_FILE *stream);
_Bool SVSH_frename(const char path[], const char new_name[]);
_Bool SVSH_fdelete(const char path[]);
