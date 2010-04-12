#include <sys/stat.h>

extern int errno;

#define READ_MODE 0
#define WRITE_MODE 1

void user_err (const char *s)
{
        printf("%s\n", s);
        exit(-1);
}

void sys_err (const char *s)
{
        perror(s);
        exit(-1);
}
