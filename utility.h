#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#ifndef __UTILITY_H__
#define __UTILITY_H__

/*
 * user_err:
 *
 * In case of an user error prints the message passed as argument and stops
 * the programs signalling an error.
 *
 * @param s:	 error message
 *
 */
void user_err(const char *s);

/*
 * sys_err:
 *
 * In case of a system error prints the message passed as argument and stops
 * the programs signalling an error.
 *
 * @param s:	 error message
 *
 */
void sys_err(const char *s);

/*
 * file_length:
 *
 * Returns the length of a file. The function take a file descriptor as
 * argument.
 *
 * @param fd:	 file descriptor
 *
 * @return:		 file length
 */
long int file_length(int fd);

#endif
