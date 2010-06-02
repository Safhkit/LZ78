#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include "utility.h"
#include <stdint.h>


#ifndef __bit_io_h__
#define __bit_io_h__

#define READ_MODE 0
#define WRITE_MODE 1
/*
 * NOTE:
 * current byte in the buffer is (n_bits / 8) and the offset is (n_bits % 8)
 */
struct bitfile{
  //file descriptor
  int fd;

  //read (0) or write (1)
  uint8_t mode;

  //size in bytes
  uint32_t bufsize;

  //WRITE_MODE: number of bits written in the buffer buf
  //READ_MODE: number of bits read from the buffer buf
  uint32_t n_bits;

  //WRITE_MODE: buffer that stores written bits and, when full, flushes to file
  //READ_MODE: buffer where bytes from file are copied and processed
  char* buf;
};

//initializes a new bitfile struct
struct bitfile* bit_open(const char* fname, uint8_t mode, uint32_t bufsize);

/**
 * reads n_bits from base and writes them to fp->buf
 *
 * @param base		from where to take bits
 * @param n_bits	number of bits to be read from base
 * @param ofs 		offset from which to read from base
 */
uint32_t bit_write(struct bitfile* fp, const char* base,
		uint32_t n_bits, int ofs);


/**
 * @param buf		where to store read bits
 * @param n_bits	number of bits to be read from fp->buf
 * @param ofs 		offset from which to write into buf
 */
uint32_t bit_read(struct bitfile* fp, char* buf, uint32_t n_bits, int ofs);

int bit_close(struct bitfile* fp);

//writes fp->buf to file until the last full byte (if less than 8 bits will
//remain, they will be written with bit_close())
uint32_t bit_flush(struct bitfile* fp);

#endif
