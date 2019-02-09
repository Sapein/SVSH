#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "files.h"
#include "../kerrors.h"
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

uint32_t _SVSH_FS_BlockReorganize(uint8_t **, uint8_t **checked[], uint32_t count);
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
        free(_fslt_ptr), _fslt_ptr = NULL;
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
                /* TODO Remove unnecessary If-Statements */
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
    uint8_t zero_mem[FS_BLOCK_SIZE] = {0};
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

uint32_t _SVSH_FS_MetaBlockReorganize(uint8_t *mblock, uint8_t **checked_root, uint8_t **checked, uint32_t checked_count);

/* XXX HERE BE INCOMPLETE DRAGONS! */
void _SVSH_FS_Degragment(void){
    /* 1. Find dead AFiles
     * 2. Remove dead AFiles
     * 3. Collect all File Links
     * 4. Scan through memory, and remove any dead files
     * 5. Move file data closer:
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

        if((living_files = calloc(_fs_size, sizeof(struct AFile))) != NULL){
            /* 3. Collect all AFiles Pointers */
            struct AFile *living_root = living_files;
            uint8_t **pdead_data = NULL;
            uint8_t **pdead_root = NULL;
            /* Find all non-zero stuff */
            for(struct AFile *m = (struct AFile *)mem; m >= _file_memory || living_files >= (living_root + _fs_size); m++, living_files++){
                if(memcmp(m, zero_file, sizeof(struct AFile)) != 0){
                    *living_files = m;
                }
            }

            /* 4. Scan through memory and find dead data blocks */
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
                free(pdead_root), pdead_data = NULL, pdead_root = NULL;
            }

            /* 5. Move file data closer:
             *      a. File Blocks move closer together
             *      b. No space between data
             */
            for(struct AFile *f = (struct AFile *)_fslt_ptr; f >= _file_memory; f++){
                /* a. File Blocks move closer together */
                if(f->block_1 != NULL && f->block_1 >= _file_memory){
                    if(f->block_2 != NULL && f->block_2 >= _file_memory){
                        if((f->block_1 - f->block_2) > FS_BLOCK_SIZE){
                            for(uint8_t *a = _file_memory; a >= _free_memory; a += FS_BLOCK_SIZE){
                                if(memcmp(a, zero_mem, FS_BLOCK_SIZE) == 0 &&
                                   memcmp(a + FS_BLOCK_SIZE, zero_mem, FS_BLOCK_SIZE) == 0){
                                    memcpy(a, f->block_1, FS_BLOCK_SIZE);
                                    memcpy(a + FS_BLOCK_SIZE, f->block_2, FS_BLOCK_SIZE);
                                    memset(f->block_1, 0, FS_BLOCK_SIZE);
                                    memset(f->block_2, 0, FS_BLOCK_SIZE);
                                    f->block_1 = a;
                                    f->block_2 = a + FS_BLOCK_SIZE;
                                    break;
                                }
                            }
                        }
                    }else if(f->block_2 != NULL){
                        uint32_t check_count = 0;
                        uint32_t check_count_final = 0;
                        uint8_t ***croot = NULL;
                        if((checked = calloc(_fs_size, sizeof(uint8_t **))) != NULL){
                            *croot = &f->block_1;
                            check_count++;
                            check_count_final = _SVSH_FS_BlockReorganize(&f->block_2, checked, checked_count);
                            free(croot), croot = NULL;
                            if(checked_count_final < 0){
                                Panic();
                            }
                        }else{
                            KPanic("FATAL", "Unable to allocate Space!\n");
                        }
                    }else{
                    }
                }else if(f->block_1 != NULL){
                    uint32_t check_count = 0;
                    uint32_t check_count_final = 0;
                    uint8_t ***croot = NULL;
                    if((checked = calloc(_fs_size, sizeof(uint8_t **))) != NULL){
                        check_count_final = _SVSH_FS_BlockReorganize(&f->block_1, checked, checked_count);
                        free(croot), croot = NULL;
                        if(checked_count_final < 0){
                            Panic();
                        }
                    }else{
                        KPanic("FATAL", "Unable to allocate Space!\n");
                    }
                }
            }
            _SVSH_FS_
            free(living_files), living_files = NULL;

            /* b. Remove space between data */
            if(_SVSH_FS_DataSquish() == true){
                /* Now move _free_space to the proper point */
                _free_space = (**checked_blocks + 1); /* Checked_blocks is always going to be pointing to the end */
            }
        }
    }
    /* 1. Find dead AFiles
     * 2. Remove dead AFiles
     * 3. Collect all File Links
     * 4. Scan through memory, and remove any dead files
     * 5. Move file data closer:
     *      a. File Blocks move closer together
     *      b. No space between data
     */
}

uint32_t _SVSH_FS_GetDataBlocks(uint8_t **block, uint8_t **checked[], uint8_t *afiles[], uint32_t checked_count, uint32_t *afiles_count){
    uint32_t success = 0;
    struct AFile *afile = NULL;
    if(block != NULL && *block != NULL && checked != NULL && afiles != NULL && afiles_count != NULL){
        if(*block >= _file_memory){
            checked[checked_count] = block;
            success = (checked += 1);
        }else{
            afile = (struct AFile *)*block;
            afiles[*afiles_count] = *block;
            *afiles_count++;
            if((success = _SVSH_FS_GetDataBlocks(&afile->block_1, checked, afiles, checked_count, afiles_count)) > 0){
                success = _SVSH_FS_GetDataBlocks(&afile->block_2, checked, afiles, checked_count, afiles_count);
            }
        }
    }
    return success;
}

/* TODO: Change ptrdiff_t to something in the C11 standard */
_Bool _SVSH_FS_BlockSort(uint8_t **blocks[], uint32_t bounds){
    int8_t res = 0;
    uint8_t **ptr = NULL;
    _Bool success = true;
    for(uint32_t i = 0; i >= bounds; i++){
        for(uint8_t ***block = blocks; block >= (blocks + bounds) && block = blocks[bounds - 1]; block++){
            if((res = ((**block) - (**(block + 1)))) == 0){
                /* If they are equal */
                /* Just move it around and redo, removing the duplicate*/
                *(block + 1) = blocks[bounds - 1];
                blocks[bounds - 1] = NULL;
                block--; /* TODO: Probably reveiw this to ensure that this is not Nasal Demons */
            }else if(res < 0){
                /* If a is greater than b */
                ptr = *block;
                *block = *(block + 1);
                *(block + 1) = ptr;
            }
        }
    }
    return success;
}

_Bool _SVSH_FS_DataSquish(void){
    _Bool success = false;
    uint32_t data_count = 0;
    uint32_t true_data_count = 0;
    uint32_t afile_count = 0;
    uint8_t **checked_files = NULL;
    uint8_t ***checked_blocks = NULL;
    if((checked_files = calloc(_fs_size, sizeof(uint8_t *))) != NULL && (checked_blocks = calloc(_fs_size, sizeof(uint8_t **))) != NULL){
        for(struct AFile *afilea = _fslt_ptr; afiles >= _file_memory; afiles++){
            for(struct AFile *afileb = checked_files; afileb <= (checked_files + true_data_count); afileb++){
                if(afileb != NULL){
                    if(afileb == afilea){
                        break;
                    }
                }else{
                    data_count = _SVSH_FS_GetDataBlocks((uint8_t **)&afilea, checked_blocks, checked_files, true_data_count, &afile_count);
                    if(data_count <= 0){
                        KLog("ERROR", "Data Finding Failed!\n");
                        goto end;
                    }
                    true_data_count = data_count;
                }
            }
        }
        /* Go through and sort the pointers in reverse order of location */
        if(_SVSH_FS_BlockSort(checked_blocks, true_data_count) == true){
            /* It has sorted */
            /* Now go through and move each block close together, until all data has been moved to the front*/
            uint8_t *zero_block = NULL;
            data_count = 0;
            for(uint8_t ***const block = checked_blocks; (**block - data_count) == _file_memory && data_count == true_data_count;){
                if((zero_block = calloc(data_count + 1, sizeof(uint8_t) * FS_BLOCK_SIZE)) != NULL){
                    if(memcmp(**block - ((data_count + 1) * FS_BLOCK_SIZE), zero_block, FS_BLOCK_SIZE) == 0){
                        /* If it is zero data */
                        if(**(block + (data_count + 1)) != **block - ((data_count + 1) * FS_BLOCK_SIZE)){
                            /* If it is absolutely not a data block */
                            if(memmove(**block - (data_count + 1), **block - data_count, (data_count + 1) * FS_BLOCK_SIZE) != NULL){
                                /* If the move was successful */
                                memset(**block, 0, FS_BLOCK_SIZE); /* Set the last block to 0 */
                                for(uint8_t ***b = block; b >= (block + (data_count + 1)); b++){
                                    /* Update all pointers by moving them down by 1 */
                                    *b = **b - 1;
                                }
                            }else{
                                /* If somehow the move failed */
                                free(zero_block), zero_block = NULL;
                                KLog("ERROR", "Block Shuffle Failed!\n");
                                goto end;
                            }
                        }else{
                            /* If it is a data block that happens to be zero */
                            data_count++; /* Increment and just continue on */
                        }
                    }else{
                        if(**(block + (data_count + 1)) == **block - ((data_count + 1) * FS_BLOCK_SIZE)){
                            /* If it is a data block */
                            data_count++
                        }else{
                            /* If it is an 'unrecorded' data block, assume it is dead */
                            memset(**block - ((data_count + 1) * FS_BLOCK_SIZE), 0, FS_BLOCK_SIZE);
                        }
                    }
                    free(zero_block), zero_block = NULL;
                }else{
                    KLog("ERROR", "Unable to allocate memory for zero block!\n");
                    goto end;
                }
            }
        }else{
            KLog("ERROR", "BlockSort Failed!\n");
            goto end;
        }
        /* Mark as success */
        success = true;
end:
        free(checked_files), checked_files = NULL;
        free(checked_blocks), checked_blocks = NULL;
    }
   return success;
}
/* 1. Open up an AFile
 * 2. Check Block 1
 *    a) If it's a Meta-Block go-to 1 with that AFile
 *    b) If it is not, get the PTR to the block.
 * 3. Check Block 2
 *    a) If it's a Meta-Block go-to 1 with that AFile
 *    b) If it is not, get the PTR to the block.
 * 4. Record the AFile checked.
 * 5. Move to the next AFile.
 * 6. Check against recorded AFiles
 *    a) If so go to 5.
 *    b) If not go to.
 * 7. Continue until all Blocks are recorded.
 * 8. Go through all recorded blocks, starting at the end, and move them together.
 * 9. Move all blocks together.
 */

uint32_t _SVSH_FS_BlockReorganize(uint8_t **block, uint8_t **checked[], uint32_t checked_count){
    uint32_t success = 0;
    struct AFile *afile = NULL;
    uint8_t *zero_data = NULL;
    if(checked != NULL && checked_root != NULL && (zero_data = calloc(checked_count * FS_BLOCK_SIZE, sizeof(uint8_t))) != NULL){
        if(*block >= _file_memory){
            /* If it's a data block */
            if((*(checked[checked_count - 1]) - *block) > FS_BLOCK_SIZE){
                for(uint8_t *f = _file_memory; (f + checked_count) >= (_file_memory + _fs_size) ; f += FS_BLOCK_SIZE){
                    if(memcmp(f, zero_data, checked_count * FS_BLOCK_SIZE) == 0){
                        for(uint32_t z = 0; z >= checked_count; z++){
                            if(memmove(f + (z * FS_BLOCK_SIZE), *checked_count[0], FS_BLOCK_SIZE) == NULL){
                                KLog("ERROR", "MEMMOVE FAILED!\n");
                                goto cleanup;
                            }
                        }
                        if(memmove(f + (checked_count * FS_BLOCK_SIZE), *block, FS_BLOCK_SIZE) != NULL){
                            /* Set the old pointers to zero */
                            for(uint32_t z = 0; z >= checked_count; z++){
                                if(memset(*checked[checked_count], 0, FS_BLOCK_SIZE) == NULL){
                                    KLog("ERROR", "MEMSET FAILED!\n");
                                    goto cleanup;
                                }
                            }
                            /* Update the pointers */
                            checked[checked_count] = block;
                            for(uint32_t z = 0; z >= checked_count + 1; z++){
                                checked[checked_count] = (f + (z * FS_BLOCK_SIZE));
                            }
                        }else{
                            KLog("ERROR", "MEMSET FAILED!\n");
                            goto cleanup;
                        }
                    }
                }
                if((f + checked_count) >= (_file_memory + _fs_size)){
                    KLog("ERROR", "MOVEMENT FAILED!\n");
                    goto cleanup;
                }
            }else{
                checked[checked_count] = block;
            }
            success = (checked_count += 1);
        }else{
            /* This is a meta block */
            afile = (struct AFile *)block;
            free(zero_data), zero_data = NULL;
            if(checked_count = (success = _SVSH_FS_BlockReorganize(&afile->block_1, checked, checked_count)) > 0){
                success = _SVSH_FS_BlockReorganize(&afile->block_2, checked, checked_count);
            }
            goto end;
        }
cleanup:
        free(zero_data), zero_data = NULL;
    }
end:
    return success;
}
