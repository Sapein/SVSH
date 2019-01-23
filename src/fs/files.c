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

/* Keep to half ot AFile */
struct EFSLT_Node {
    uint32_t file_size; /* 4 bytes, addr / 4 */
    uint8_t *efslt_ptr; /* 4 or 6 bytes */
}; /* Size = 8 bytes (32 bit) ; 10 bytes (64 bit) */

struct EFSLT_Link {
    uint32_t parent_size;
    uint8_t *parent;
};

uint32_t _SVSH_FS_FSLTCreate(uint32_t fs_size);
void *_SVSH_FS_EFSLTCreate(uint32_t size, uint8_t *parent);
void _SVSH_FS_EFSLTDestroy(uint8_t *efslt, uint32_t efslt_size);
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
    struct EFSLT_Node *node = NULL;
    if((fs_size / FS_FSLT_SIZE) > (sizeof(struct AFile) + (2 * sizeof(struct EFSLT_Node)))){
        fslt_size = fs_size / FS_FSLT_SIZE;
        fslt += fslt_size;
        fslt -= sizeof(struct EFSLT_Node) * 2;
        node = (struct EFSLT_Node *)fslt;
        node->file_size = 0;
        node->efslt_ptr = NULL;
        fslt += sizeof(struct EFSLT_Node);
        node = (struct EFSLT_Node *)fslt;
        node->file_size = 0;
        node->efslt_ptr = NULL;
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

void *_SVSH_FS_EFSLTCreate(uint32_t size, uint8_t *parent){
    uint8_t *efslt_ptr = NULL;
    struct EFSLT_Node *node = NULL;
    struct EFSLT_Link *link = NULL;
    if(_free_memory != NULL){
        _free_memory += size;
        efslt_ptr = _free_memory - size;
        efslt_ptr += size;
        node = (struct EFSLT_Node *)efslt_ptr;
        node -= 2;
        node->file_size = 0;
        node->efslt_ptr = NULL;
        node += 1;
        node->file_size = 0;
        node->efslt_ptr = NULL;
        efslt_ptr -= size;
        link = (struct EFSLT_Link *)efslt_ptr;
        if(parent != NULL){
            uint8_t *sp = NULL;
            uint32_t sp_size = 0;
            if(parent != _fslt_ptr){
                sp_size = ((struct EFSLT_Link *)parent)->parent_size;
                sp = ((struct EFSLT_Link *)parent)->parent;
                if(sp != _fslt_ptr){
                    sp += sp_size;
                    sp -= sizeof(struct EFSLT_Link *) * 2;
                    if(((struct EFSLT_Node *)sp)->efslt_ptr == parent){
                        link->parent_size = ((struct EFSLT_Node *)sp)->file_size;
                    }
                    sp += sizeof(struct EFSLT_Link *);
                    if(((struct EFSLT_Node *)sp)->efslt_ptr == parent){
                        link->parent_size = ((struct EFSLT_Node *)sp)->file_size;
                    }else{
                        memset(efslt_ptr, 0, size);
                        if((efslt_ptr+size) == _free_memory){
                            _free_memory = efslt_ptr;
                        }
                        efslt_ptr = NULL;
                    }
                }else{
                    link->parent_size = (_file_memory - _fslt_ptr);
                }
            }else{
                    link->parent_size = (_file_memory - _fslt_ptr);
            }
            link->parent = parent;

        }else{
            link->parent_size = (_file_memory - _fslt_ptr);
            link->parent = _fslt_ptr;
        }
    }
    return efslt_ptr;
}

void _SVSH_FS_EFLSTDestroy(uint8_t *efslt, uint32_t efslt_size){
    struct EFSLT_Link *link = (struct EFSLT_Link *)efslt;
    struct EFSLT_Node *node = NULL;
    uint8_t *scrubb_ptr = NULL;
    if(link != NULL){
        scrubb_ptr = link->parent;
        node = (struct EFSLT_Node *)(scrubb_ptr + link->parent_size);
        node -= 2;
        if(node->efslt_ptr != NULL){
            if(node->efslt_ptr == efslt){
                node->efslt_ptr = NULL;
                if(efslt_size != node->file_size){
                    efslt_size = node->file_size;
                }
                node->file_size = 0;
                if((node + 1)->efslt_ptr != NULL){
                    node->efslt_ptr = (node + 1)->efslt_ptr;
                    node->file_size = (node + 1)->file_size;
                    node++;
                    goto remove;
                }
            }
            node++;
            if(node->efslt_ptr == efslt){
remove:
                node->efslt_ptr = NULL;
                if(efslt_size != node->file_size){
                    efslt_size = node->file_size;
                }
                node->file_size = 0;
            }

        }
        scrubb_ptr = NULL;
        node = NULL;
        *efslt += efslt_size;
        node = (struct EFSLT_Node*)efslt;
        node -= 2;
check:
        if(node->efslt_ptr != NULL){
            _SVSH_FS_EFLSTDestroy(node->efslt_ptr, node->file_size);
            node++;
            goto check;
        }
        memset(efslt, 0, efslt_size);
        /* We will want to Defragment after this, but for now we won't, so it can be left to the caller.*/
    }
}

/* blocks_out is either 0, 1, 2, or 3
 * 0 = No Blocks
 * 1 = 1 Block
 * 2 = 2 Blocks
 * 3 = 1 Block, but incorrectly set
 */
_Bool _SVSH_FS_AnyEFSLT(uint8_t *blocks_out){
    _Bool efslt_exists = false;
    *blocks_out = 0;
    if((struct EFSLT_Node *)(_file_memory - (sizeof(EFSLT_Node) * 2))->parent != NULL){
        efslt_exists = true;
        if((struct EFSLT_Node *)(_file_memory - sizeof(EFSLT_Node))->parent != NULL){
            *blocks_out = 2;
        }else{
            *blocks_out = 1;
        }
    }else if((struct EFSLT_Node *)(_file_memory - sizeof(EFSLT_Node))->parent != NULL){
        /* There is an EFSLT that's not linked properly */
        *blocks_out = 3;
    }
    return efslt_exists;
}

_Bool SVSH_FS_Defragment(void){
    _Bool success = false;
    uint8_t *mem = NULL;
    struct AFile *f = NULL;
    struct AFile zero_f = {0};
    uint8_t *zero_block = NULL;
    uint8_t *dead_files = calloc(_fs_size, sizeof(uint8_t));
    _Bool efslt_exists = false;
    uint8_t efslt_t = 0;
    /* We absolutely need to lock access to memory here if we thread this */

    efslt_exists = _SVSH_FS_AnyEFSLT(*efslt_t);

    if(_fslt_ptr != NULL && ((zero_block = calloc(1, FS_BLOCK_SIZE)) != NULL)){
        mem = _fslt_ptr;

        while((efslt_exists = _SVSH_FS_AnyEFSLT(&efslt_t)) == true && efslt_t == 3){
            /* Check and fix-up the EFSLT links in the FSLT */
            if(memcmp(memcpy((_file_memory - (sizeof(EFSLT_Node) * 2)),
                            (_file_memory - sizeof(EFSLT_Node)),
                            sizeof(EFSLT_Node)),
                        (_file_memory - sizeof(EFSLT_Node))) == 0){
                memset((_file_memory - sizeof(EFSLT_Node)), 0, sizeof(EFSLT_Node));
            }
        }

        /* Find open/dead memory */
        for(int i = 0, f = (struct AFile *)mem; (uint8_t *)f == _file_memory || i >= _fs_size; f++){
            if(memcmp(f, &zero_f, sizeof(struct AFile)) != 0){
                if(f->block_2 != NULL && f->block_1 == NULL){
                    /* Move block_2 to block_1, if block_1 is NULL */
                    f->block_1 = f->block_2;
                    f->block_2 = NULL;
                }

                if(f->block_1 != NULL){
                    if(f->block_1 > _file_memory && efslt_exists){
                        /* If it is greater than _file_memory and there is are EFSLT */
                        /* Then check through the EFSLT's */
                        ;
                    }else{
                        /* There are no EFSLT's, so it is NOT a meta-block */
                        /* If there are no EFSLT's, and Block 1 is not a meta-block, check the next block */
                        if(memcmp(f->block_1, zero_block, FS_BLOCK_SIZE) == 0){
                            /* This is a Zero block, so we need to check block_2 */
                            if(f->block_2 != NULL && f->block_2 > _file_memory && memcmp(f->block_2, zero_block, FS_BLOCK_SIZE) == 0){
                                /* This is also a Zero Block, so the AFile is a dead file */
                                f->block_1 = NULL;
                                f->block_2 = NULL;
                                dead_files[i] = (uint8_t *)f;
                                i++;
                            }else if(f->block_2 != NULL && f->block_2 > _file_memory){
                                /* Block_2 IS NOT a Zero Block */
                                f->block_1 = f->block_2;
                                f->block_2 = NULL;
                            }else if(f->block_2 != NULL && f->block_2 
                        }else{
                            /* This is not a zero block, so we can check block_2 */
                            if(f->block_2 != NULL && f->block_2 > _file_memory && memcmp(f->block_2, zero_block, FS_BLOCK_SIZE) == 0){
                                /* This is also a Zero Block, so the AFile is a dead file */
                                dead_files[i] = (uint8_t *)f;
                                i++;
                            }
                        }
                    }else{
                        /* It IS a meta_block */
                    }

                    /* Check to see if block_2 is not NULL, and if it isn't a 'meta-block' */
                }else{
                    /* The block is dead, so mark as dead and then move on */
                    dead_files[i] = (uint8_t *)f;
                    i++;
                }
            }
        }

        /* Set open/dead memory to zero */

    }
    free(dead_files);
    return true;
}
