#include <malloc.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include "utility.h"
#include <stdint.h>


#ifndef __bit_io_h__
#define __bit_io_h__

/*
 * NOTA: per l'offset all'interno del buffer si usa (n_bits % 8)
 * per accedere al byte corretto, invece, si usa (n_bits / 8)
 *
 */
struct bitfile{
  int fd;			//descrittore file
  uint8_t mode; 		//0: r, 1: w
  uint32_t bufsize;		//dimensione in byte del buffer
  uint32_t n_bits;		//numero bit attualmente scritti/letti nel/dal buffer
  char* buf;		//buffer per I/O bufferizzato
};

/*
Inizializza la struttura: viene aperto un file in lettura o scrittura,
viene specificata la dimensione del buffer di lavoro.
Il buffer di lavoro nelle operazioni di scrittura dovrà essere riempito con
i bit letti da un buffer di ingresso (base) e quando pieno si dovrà
scrivere il suo contenuto sul file di uscita (fd).
Nelle operazioni di lettura, dovrà essere riempito con i bit letti da file
e quando pieno si dovrà scrivere sul buffer passato (buf).
*/
struct bitfile* bit_open(const char* fname, uint8_t mode, uint32_t bufsize);

/**
 * @param base		buffer da cui leggere
 * @param n_bits	numero di bit da leggere dal buffer
 * @param ofs 		offset da cui leggere da base (base allineato al byte)
 */
uint32_t bit_write(struct bitfile* fp, const char* base, uint32_t n_bits, int ofs);


/**
 * @param buf		buffer su cui scrivere il risultato della lettura
 * @param n_bits	numero di bit da leggere dal file (chiamante deve allocare
 *                  abbastanza spazio su buf)
 * @param ofs 		offset di buf da cui scrivere i bit
 */
uint32_t bit_read(struct bitfile* fp, char* buf, uint32_t n_bits, int ofs);

int bit_close(struct bitfile* fp);

/**
 * Quando la bit_write ha scritto tutto il buffer di lavoro
 * viene fatta la flush che scrive su file.
 * Quando la bit_read ha scritto tutto il buffer di lavoro,
 * viene fatta la scrittura nel buffer passato (buf).
 */
uint32_t bit_flush(struct bitfile* fp);

#endif
