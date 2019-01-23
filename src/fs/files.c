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

uint8_t *_SVSH_FS_TraverseMetaBlocks(uint8_t *mem, _Bool efslt_exists, uint8_t *original_mem){
    uint8_t *_actual_data_block = NULL;
    struct AFile *amem = (struct AFile *)mem;
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
        /* Add this in later */
        if(mem >= _file_memory){
traverse_block_check_b:
            /* This is potentially regular data, or EFSLT data, we need to check. */
            /* What we need to do is get each EFSLT that exists and store it's size and location,
             *  then we can check the address of mem (and it's blocks) against the EFSLT's.
             * If the addresses are in between the EFSLT space (for any one of them), then we
             *  know it's a meta-block and need to check it. */
#error Actually Write the EFSLT stuff
        }else if(amem_block->block_1 == NULL && amem->block_2 == NULL){
            _actual_data_block = NULL;
        }else{
traverse_block_check_c:
            if(amem->block_1 != NULL){
                if(amem_block->block_1 < _file_memory){
                    actual_data_block = _SVSH_FS_TraverseMetaBlocks(amem->block_1, efslt_exists, original_mem);
                }else{
                    goto traverse_block_check_efslt_b;
                }
            }else if(amem->block_2 != NULL){
                amem->block_1 = amem->block_2;
                amem->block_2 = NULL;
                goto traverse_block_check_c;
            }
            if(amem->block_2 != NULL && amem->block_1 != NULL && _actual_data_block == NULL){
                if(amem->block_2 < _file_memory){
                    _actual_data_block = _SVSH_FS_TraverseMetaBlocks(amem->block_2, efslt_exists, original_mem);
                }else{
                    /* Insert code here */
#error Actually write the code to deal with the second block.
                }
            }
        }
    }
    return _actual_data_block;
}

void _SVSH_FS_Degragment(void){
    /* 1. Find dead AFiles
     * 2. Remove dead AFiles
     * 3. Move AFiles closer together.
     * 4. Remove Dead/Unnecessary EFSLT's
     * 5. Collect all File Links
     * 6. Scan through memory, and remove any dead files
     * 7. Move file data closer:
     *      a. EFSLT's move closer to FSLT's
     *      b. File Blocks move closer together
     *      c. No space between data
     */
    _Bool efslt_exists = false;
    uint8_t efslt_links = 0;
    uint8_t *mem = _fslt_ptr;
    uint8_t *scratch = NULL;
    struct AFile *dead_afiles = NULL;
    struct AFile zero_file = {.block_1 = NULL, .block_2 = NULL, .permissions=0, .name={0}, .file_size=0};
    uint8_t *zero_file = NULL;

    if(_fslt_ptr != NULL && (((zero_file = calloc(1, sizeof(FS_BLOCK_SIZE)) != NULL))
                          && ((dead_afiles = calloc(_fs_size, sizeof(struct AFile)))))){
        /* Check for efslt's existing, if they exist, then check for issues and fix */
        while((efslt_exists = _SVSH_FS_AnyEFSLT(*efslt_links)) && efslt_links == 3){
            if(memcmp((struct EFSLT_Node *)(_file_memory - sizeof(EFSLT_Node) * 2),
                        memcpy((struct EFSLT_Node *)(_file_memory - sizeof(EFSLT_Node) * 2),
                            (struct EFSLT_Node *)(_file_memory - sizeof(EFSLT_Node)),
                            sizeof(EFSLT_Node)),
                        sizeof(struct EFSLT_Node)) == 0){
                memset((struct EFSLT_Node *)(_file_memory - sizeof(EFSLT_Node)), 0, sizeof(EFSLT_Node));
            }
        }

        /* Step 1 - Find (and mark) dead AFiles. */
        for(int i = 0; mem == _file_memory || i >= _fs_size; mem = mem + sizeof(struct AFile)){
            struct AFile *temp_afile = (struct AFile *)mem;
            if(memcmp(temp_afile, &zero_file, sizeof(struct AFile)) != 0){
                /* If the AFile exists */

                if(efslt_exists){
                    /* Can't assume anything about blocks now, as they might point to efslt's */
block_1_check_a:
                    if(temp_afile->block_1 != NULL){
                        if(temp_afile->block_1 >= _file_memory){
                            /* It may or may not be an EFSLT, we have to check */

                            /* Check if it's a Meta-Block */
                            /* If it is follow all the way down (goto here?) */
                            /* Otherwise, it's a regular file so check and make sure there
                             * are no zero blocks, and that block_1 is not equal to block_2.
                             */
                        }else{
                            /* This is definitely a Meta-Block */
                            /* So now we have to follow all the way down */
                        }
                    }else if(temp_afile->block_2 != NULL){
                        /* If the second block exists, but not the first */
                        temp_afile->block_1 = temp_afile->block_2;
                        temp_afile->block_2 = NULL;
                        goto block_1_check_a;
                    }
                }else{
                    /* Can assume that if they aren't in the fslt space, then they are not 'meta-blocks' */
block_1_check_b:
                    if(temp_afile->block_1 != NULL){
                        if(temp_afile->block_1 >= _file_memory){
                            /* It's a regular file block */
                            if(memcmp(temp_afile->block_1, zero_file, sizeof(FS_BLOCK_SIZE)) == 0){
                                /* If it's a Zero block, then we need to check Block 2 */
                                if(temp_afile->block_2 != NULL){
                                    if(temp_afile->block_2 != temp_afile->block_1){
                                        temp_afile->block_1 = temp_afile->block_2;
                                        temp_afile->block_2 = NULL;
                                        goto block_1_check_b;
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
                            if((scratch = _SVSH_FS_TraverseMetaBlocks(temp_afile->block_1, efslt_exists, NULL)) == NULL){
                                /* If this is a data-less meta-block (or a looped meta-block) ...*/
                                if(temp_afile->block_2 != NULL){
                                    /* Check the next block */
                                    if(temp_afile->block_2 >= _file_memory){
                                        /* If it's not a meta-block */
                                        temp_afile->block_1 = temp_afile->block_2;
                                        goto block_1_check_b;
                                    }else{
                                        /* Check this one as well */
                                        if((scratch = _SVSH_FS_TraverseMetaBlocks(temp_afile->block_2, efslt_exists, NULL)) == NULL){
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
                                     _SVSH_FS_TraverseMetaBlocks(temp_afile->block_2, efslt_exists, NULL) != NULL){
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
        }
    }
}
