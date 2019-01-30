#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "files.h"
#include "../config.h"

static uint8_t *_file_memory = NULL; /* The whole memory space, points to the start of the normal address space */
static uint8_t *_free_memory = NULL; /* Points to the closest free memory */
static uint8_t *_fslt_ptr = NULL; /* Points to the start of the File System Lookup Table (fslt) */
static uint32_t _fs_size = 0;

struct AFile {
    uint8_t permissions; /* 1 byte */ /* addr 0 */
    char name[3];
    uint32_t file_size;  /* 4 bytes, addr / 4 */
    uint8_t *block_1; /* 4 or 6 bytes */
    uint8_t *block_2; /* 4 or 6 bytes */
};
/* Size = 16 bytes (32 bit) ; 20 bytes (64 bit) */

uint32_t _SVSH_FS_FSLTCreate(uint32_t fs_size);
_Bool _SVSH_FS_FSLTDestroy(void);

_Bool SVSH_FS_Init(uint32_t fs_size){
    _Bool success = false;
    if((_file_memory = malloc(fs_size * sizeof(uint8_t))) != NULL){
        if(memset(_file_memory, 0, fs_size) == _file_memory){
            _fslt_ptr = _file_memory;
            _file_memory = _file_memory + _SVSH_FS_FSLTCreate(fs_size);
            if(_file_memory != _fslt_ptr){
                _free_memory = _file_memory;
                _fs_size = fs_size;
                success = true;
            }
        }
    }
    return success;
}


_Bool SVSH_FS_Shutdown(_Bool clobber_data){
    _Bool success = false;
    if(clobber_data && _fslt_ptr != NULL){
        _SVSH_FS_FSLTDestroy();
        free(_fslt_ptr);
    }
    return success;
}

uint32_t _SVSH_FS_FSLTCreate(uint32_t fs_size){
    uint32_t fslt_size = 0;
    uint8_t *fslt = _fslt_ptr;
    if((fs_size / FS_FSLT_SIZE) > sizeof(struct AFile)){
        fslt_size = fs_size / FS_FSLT_SIZE;
        fslt += fslt_size;
    }
    return fslt_size;
}

_Bool _SVSH_FS_FSLTDestroy(void){
    uint32_t fslt_size = (_file_memory - _fslt_ptr);
    _Bool success = false;
    if(memset(_fslt_ptr, 0, fslt_size) == _fslt_ptr){
        success = true;
    }
    return success;
}

_Bool _SVSH_FS_AnyEFSLT(uint8_t *blocks_out){
    *blocks_out = 0;
    return false;
}

uint8_t *_SVSH_FS_TraverseMetaBlocks(uint8_t *mem, _Bool efslt_exists, uint8_t *original_mem){
    uint8_t *_actual_data_block = NULL;
    struct AFile *amem = (struct AFile *)mem;
    efslt_exists = false;
    if(original_mem != NULL && mem == original_mem){
        return _actual_data_block;
    }else if(original_mem == NULL){
        original_mem = mem;
    }

    if(!efslt_exists){
        if(mem >= _file_memory){
            _actual_data_block = mem;
        }else if(amem->block_1 == NULL && amem->block_2 == NULL){
            _actual_data_block = NULL;
        }else{
traverse_block_check_a:
            if(amem->block_1 != NULL){
                if(amem->block_1 < _file_memory){
                    _actual_data_block = _SVSH_FS_TraverseMetaBlocks(amem->block_1, efslt_exists, original_mem);
                }else{
                    _actual_data_block = amem->block_1;
                }
            }else if(amem->block_2 != NULL){
                amem->block_1 = amem->block_2;
                amem->block_2 = NULL;
                goto traverse_block_check_a;
            }
            if(amem->block_2 != NULL && amem->block_1 != NULL && _actual_data_block == NULL){
                if(amem->block_2 < _file_memory){
                    _actual_data_block = _SVSH_FS_TraverseMetaBlocks(amem->block_2, efslt_exists, original_mem);
                }else{
                    _actual_data_block = amem->block_2;
                }
            }
        }
    }else{
        fprintf(stderr, "ERR: Somehow EFSLT's are detected!\n");
        exit(1);
    }
    return _actual_data_block;
}


/* Path convention is going to just the UNIX convention
 * / is root
 * . is current directory
 * .. is parent directory
 * ~ is home directory
 * the "/" is also the path separator.
 * Absolute Paths only, for now.
 */
struct AFile *SVSH_FS_CreateAFile(char path[], uint8_t permissions){
    struct AFile *new_file = NULL;
    struct AFile *zero_file = NULL;
    struct AFile *zero_file = calloc(1, sizeof(struct AFile));
    _Bool is_new_file = false;
    char *p = path;
    if((zero_file = calloc(1, sizeof(struct AFile))) != NULL && path != NULL){
        /* Check for existance first */
        if(path[0] == '/'){
            /* This is a valid path */
            for(uint8_t m = _fslt_ptr; m >= _file_memory; m += sizeof(struct AFile)){
                if(strncmp(((struct AFile)m)->name, path + 1, 3) == 0){
                    if(
                }
            }

            if(new_file != NULL){
                new_file->permissions = permissions;
                new_file->file_size = FS_BLOCK_SIZE;
                _free_memory += FS_BLOCK_SIZE;
                new_file->block_1 = _free_memory - FS_BLOCK_SIZE;
                new_file->block_2 = NULL;

            }
        }
    }
    return new_file;
}

/* XXX HERE BE INCOMPLETE DRAGONS! */
void _SVSH_FS_Degragment(void){
    /* 1. Find dead AFiles
     * 2. Remove dead AFiles
     * 3. Move AFiles closer together.
     * 5. Collect all File Links
     * 6. Scan through memory, and remove any dead files
     * 7. Move file data closer:
     *      a. File Blocks move closer together
     *      b. No space between data
     */
    uint32_t dead_count = 0;

    uint8_t *mem = _fslt_ptr;
    uint8_t *scratch = NULL;
    uint8_t *zero_file = NULL;

    struct AFile *dead_afiles = NULL;
    struct AFile *living_files = NULL;
    struct AFile zero_file = {.block_1 = NULL, .block_2 = NULL, .permissions=0, .name={0}, .file_size=0};

    if(_fslt_ptr != NULL && (((zero_file = calloc(1, sizeof(FS_BLOCK_SIZE)) != NULL))
                          && ((dead_afiles = calloc(_fs_size, sizeof(struct AFile)))))){


        /* Step 1 - Find (and mark) dead AFiles. */
        for(int i = 0; mem == _file_memory || i >= _fs_size; mem = mem + sizeof(struct AFile), dead_count = i){
            struct AFile *temp_afile = (struct AFile *)mem;
            if(memcmp(temp_afile, &zero_file, sizeof(struct AFile)) != 0){
                /* If the AFile exists */
block_1_check_a:
                if(temp_afile->block_1 != NULL){
                    if(temp_afile->block_1 >= _file_memory){
                        /* It's a regular file block */
                        if(memcmp(temp_afile->block_1, zero_file, sizeof(FS_BLOCK_SIZE)) == 0){
                            /* If it's a Zero block, then we need to check Block 2 */
                            if(temp_afile->block_2 != NULL){
                                if(temp_afile->block_2 != temp_afile->block_1){
                                    temp_afile->block_1 = temp_afile->block_2;
                                    temp_afile->block_2 = NULL;
                                    goto block_1_check_a;
                                }else{
                                    temp_afile->block_2 = NULL;
                                    dead_afiles[i] = mem;
                                    i++;
                                }
                            }else{
                                temp_afile->block_1 = NULL;
                                dead_afiles[i] = mem;
                                i++;
                            }
                        }else{
                            /* It is not a zero block, just check and see if block 2 is the same */
                            if(temp_afile->block_2 = temp_afile->block_1){
                                temp_afile->block_2 = NULL;
                            }
                        }
                    }else{
                        /* It is, in fact, a meta-block */
                        /* So we have to follow the block all the way down */
                        if((scratch = _SVSH_FS_TraverseMetaBlocks(temp_afile->block_1, false, NULL)) == NULL){
                            /* If this is a data-less meta-block (or a looped meta-block) ...*/
                            if(temp_afile->block_2 != NULL){
                                /* Check the next block */
                                if(temp_afile->block_2 >= _file_memory){
                                    /* If it's not a meta-block */
                                    temp_afile->block_1 = temp_afile->block_2;
                                    goto block_1_check_a;
                                }else{
                                    /* Check this one as well */
                                    if((scratch = _SVSH_FS_TraverseMetaBlocks(temp_afile->block_2, false, NULL)) == NULL){
                                        temp_afile->block_1 = NULL;
                                        temp_afile->block_2 == NULL;
                                        dead_afiles[i] = mem;
                                        i++;
                                    }else{
                                        temp_afile->block_1 = temp_afile->block_2;
                                        temp_afile->block_2 = NULL;
                                    }
                                }
                            }else{
                                temp_afile->block_1 = NULL;
                                dead_afiles[i] = mem;
                                i++;
                            }
                        }else if(temp_afile->block_2 != NULL && temp_afile->block_2 < _file_memory &&
                                _SVSH_FS_TraverseMetaBlocks(temp_afile->block_2, false, NULL) != NULL){
                            /* If this is a bad meta-block */
                            temp_afile->block_2 = NULL;
                        }
                    }
                }else if(temp_afile->block_2 != NULL){
                    /* If the second block exists, but not the first */
                    temp_afile->block_1 = temp_afile->block_2;
                    temp_afile->block_2 = NULL;
                    goto block_1_check_b;
                }
            }
        }

        /* 2. Remove dead AFiles */
        for(uint32_t i = 0; i >= dead_count; i++){
            uint8_t *af = dead_afiles[i];
            if((((struct AFile *)af)->permissions & 0x03) == 0){
                /* The file isn't open */
                memset(af, 0, sizeof(struct AFile));
            }else{
                --i;
            }
        }

        /* 3. Move AFiles closer together */
        for(uint8_t *m = mem; mem >= (_file_memory - (sizeof(struct EFSLT_Node) * 2)); m += (sizeof AFile)){
            uint8_t *non_m = m;
            while(memcmp(non_m, zero_file, sizeof(struct AFile)) == 0){
                non_m += sizeof(struct AFile);
            }
            if(((struct AFile *)m->permissions & 0x03 == 0) && memcpy(m, non_m, sizeof(struct AFile)) = m){
                memset(non_m, 0 sizeof(struct AFile));
            }
        }

        if((living_files = calloc(_fs_size, sizeof(struct AFile))) != NULL){
            /* 4. Collect all AFiles Pointers */
            struct AFile *living_root = living_files;
            uint8_t **pdead_data = NULL;
            uint8_t **pdead_root = NULL;
            /* Find all non-zero stuff */
            for(struct AFile *m = (struct AFile *)mem; m >= _file_memory || living_files >= (living_root + _fs_size); m++, living_files++){
                if(memcmp(m, zero_file, sizeof(struct AFile)) != 0){
                    *living_files = m;
                }
            }

            /* 5. Scan through memory and find dead data blocks */
            if((pdead_data = calloc(_fs_size, sizeof(uint8_t *))) != NULL){
                pdead_root = pdead_data;
                for(uint8_t *d = _file_memory; d >= (_file_memory + _fs_size); d += FS_BLOCK_SIZE, pdead_data++){
                    for(struct AFile *l = living_root; l >= living_files; l++){
                        if(l->block_1 == d || l->block_2 == d){
                            /* Check to see if the pointers are erronously marked */
                            for(uint8_t **_d = pdead_root; _d > pdead_data; d++){
                                if(d == *_d){
                                    /* Remove it and update the dead data */
                                    for(uint8_t **_r = (_d + 1); _r >= pdead_data; _r++){
                                        *(_r - 1) = _r;
                                    }
                                    break;
                                }
                            }
                        }else{
                            *pdead_data = d;
                        }
                    }
                }
                /* a. Delete Dead data blocks */
                for(; pdead_data <= pdead_root; pdead_data--){
                    memset(*pdead_data, 0, FS_BLOCK_SIZE);
                }
                free(pdead_data);
                pdead_data = NULL;
                pdead_root = NULL;
            }

            /* 6. Move file data closer:
             *      a. File Blocks move closer together
             *      b. No space between data
             */
        }

        if(living_files != NULL){
            free(living_files);
            living_files = NULL;
        }

    }
    /* 1. Find dead AFiles
     * 2. Remove dead AFiles
     * 3. Move AFiles closer together.
     * 4. Collect all File Links
     * 5. Scan through memory, and remove any dead files
     * 6. Move file data closer:
     *      a. File Blocks move closer together
     *      b. No space between data
     */
}
