#ifndef SVSH_IO
#define SVSH_IO

struct SVSH_FILE;
typedef struct SVSH_FILE SVSH_FILE;
SVSH_FILE *SVSH_fopen(const char path[], const char mode[]);
int SVSH_fclose(SVSH_FILE *file);

size_t SVSH_fwrite(const void *input, size_t size, size_t nmemb, SVSH_FILE *stream);
size_t SVSH_fread(void *output, size_t size, size_t nmemb, SVSH_FILE *stream);
_Bool SVSH_frename(const char path[], const char new_name[]);
_Bool SVSH_fdelete(const char path[]);
#endif
