#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "utility.h"

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

long int file_length(int fd)
{
	struct stat buf;
	fstat(fd, &buf);
	return buf.st_size;
}
