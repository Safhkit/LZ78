#include <malloc.h>
#include <stdio.h>
#include <strings.h>

#ifndef __bit_io_h__
#define __bit_io_h__

struct bitfile{
  int fd;
  int mode; //r, w
  int bufsize;
  //int w_inizio;
  int n_bits; //numero di bit *attualmente* contenuti
  //int ofs;
  char buf[0];
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
struct bitfile* bit_open(const char* fname, int mode, int bufsize);

/**
* @param base buffer da cui leggere
* @param n_bits numero di bit da leggere dal buffer
* @param ofs offset da cui leggere da base (base allineato al byte)
*/
int bit_write(struct bitfile* fp, const char* base, int n_bits, int ofs);

/**
 * @param buf buffer su cui scrivere il risultato della lettura
 * @param n_bits numero di bit da leggere dal file (chiamante deve allocare
 * 													abbastanza spazio su buf)
 * @param ofs offset di buf da cui scrivere i bit
 */
int bit_read(struct bitfile* fp, char* buf, int n_bits, int ofs);

int bit_close(struct bitfile* fp);

/**
 * Quando la bit_write ha scritto tutto il buffer di lavoro o si sono
 * letti n_bits da base, viene fatta la flush che scrive su file.
 * Quando la bit_read ha scritto tutto il buffer di lavoro, o si sono letti
 * da file n_bits, viene fatta la scrittura nel buffer passato (buf).
 */
int bit_flush(struct bitfile* fp);

#endif
