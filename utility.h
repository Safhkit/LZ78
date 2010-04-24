#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef __UTILITY_H__
#define __UTILITY_H__

extern int errno;

#define READ_MODE 0
#define WRITE_MODE 1

void user_err(const char *s);
void sys_err(const char *s);

#endif
