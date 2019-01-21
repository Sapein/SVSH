#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "files.h"

static uint8_t *_root_mem = NULL;
static uint8_t *_free_memory = NULL;
static struct SVSH_FS_File *_files = NULL;
struct SVSH_FS_File {
    uint32_t file_size; /* This is the size of the file in Bytes for DATA and child count for FOLDERS */
    uint8_t permissions; /* Permission layout is as follows (in order of bytes RWX0) */
    enum SVSH_FS_FileType type;
    char name[sizeof(uint32_t)];
    struct SVSH_FS_File *parent;
    void **data;
};

struct SVSH_FS_Folder { /*TODO: Make this actually work better with the way we have things structured*/
    struct SVSH_FS_File *children;
};

struct SVSH_FS_Data {
    char *data;
};

_Bool SVSH_FS_Init(uint32_t filesystem_size){
    _Bool success = false;
    uint8_t *offset_ptr = NULL;
    struct SVSH_FS_File root = {.file_size = filesystem_size, .permissions = 0xFF, .type = ROOT,
                                .data = NULL, .parent = NULL, .name={0}};
    struct SVSH_FS_Folder *f = NULL;
    if((_free_memory = malloc(filesystem_size * sizeof(uint8_t))) != NULL){
        if(memset(_free_memory, 0, filesystem_size) == _free_memory){
            _root_mem = _free_memory, offset_ptr = _free_memory;
            offset_ptr += sizeof(struct SVSH_FS_File);
            f = (struct SVSH_FS_Folder *)offset_ptr;
            f->children = NULL;
            root.data = (void *)&f;
            if(memcpy(_free_memory, &root, sizeof(struct SVSH_FS_File)) == _free_memory){
                _files = (struct SVHS_FS_File *)_free_memory; /* Move root over to _files */
                _free_memory  = _free_memory + sizeof(struct SVSH_FS_File);
                _free_memory = _free_memory + sizeof(struct SVSH_FS_Folder); /* Increase _free_memory's pointer to the next free space */
                offset_ptr = NULL;
                success = true;
            }
        }
    }
    return success;
}

_Bool SVSH_FS_Shutdown(_Bool clobber_data){
    _Bool success = false;
    if(clobber_data && _free_memory != NULL){
        success = true;
        free(_free_memory);
        _free_memory = NULL;
    }
    return success;
}
