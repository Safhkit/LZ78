#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#ifndef __UTILITY_H__
#define __UTILITY_H__

void user_err(const char *s);
void sys_err(const char *s);
long int file_length(int fd);

#endif
