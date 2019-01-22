#ifndef SVSH_FILES
#include <stdint.h>
#define SVSH_FILES
#define READABLE 0xC0
#define WRITABLE 0x30
#define EXECUTABLE 0x0C
#define SVSH_FS_HALT() SVSH_FS_Shutdown(0);

struct SVSH_FS_File;
enum SVSH_FS_FileType { DATA, FOLDER, ROOT };
typedef struct SVSH_FS_File SVSH_FS_File;
typedef struct SVSH_FS_Permissions {
    _Bool is_readable;
    _Bool is_writable;
    _Bool is_executable;
} SVSH_FS_Permissions;

/* SVSH_FS_Init
 * Args: filesystem_size (uint32_t)
 * Returns: Success (Bool)
 *
 * This initalizes the SVSH Psuedo-Filesystem to the specified size.
 * The size provided is in the number of Bytes (8 bits) to allocate.
 */
_Bool SVSH_FS_Init(uint32_t filesystem_size);

/* SVSH_FS_Shutdown
 * Args: Clobber Data (Bool)
 * Returns: Success (Bool)
 *
 * This shutsdown the SVSH Psuedo-Filesystem after initing.
 * If clobber_data is set, the return value can be ignored, otherwise
 *  the return value signifies that files are still reading
 */
_Bool SVSH_FS_Shutdown(_Bool clobber_data);

struct SVSH_FS_File *SVSH_FS_OpenFile(char name[sizeof(uint32_t)], uint32_t perms);
void SVSH_FS_CloseFile(SVSH_FS_File *file);
#endif
