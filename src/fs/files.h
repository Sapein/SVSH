#ifndef SVSH_FILES
#include <stdint.h>
#define SVSH_FILES
#define SVSH_KB_TO_B(x) ((x)*1000)
#define SVSH_MB_TO_B(x) ((x)*1000000)
#define SVSH_GB_TO_B(x) ((x)*1000000000)
#define SVSH_KIB_TO_B(x) ((x)*1024)
#define SVSH_MIB_TO_B(x) ((x)*2048)
#define SVSH_GIB_TO_B(x) ((x)*3072)
#define SVSH_FS_HALT() SVSH_FS_Shutdown(0);

struct SVSH_FS_File;
enum SVSH_FS_FileType { DATA, FOLDER, ROOT };
typedef struct SVSH_FS_File SVSH_FS_File;

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
#endif
