#include "bit_io.h"


//TODO: free di bf

bitfile* bit_open(const char* fname, int mode, int bufsize)
 {
	FILE* ff;
	struct bitfile* bf = (struct bitfile*)malloc(sizeof(bitfile));
	if (bf == NULL) {
		//TODO: gestione errori
	}
	if (mode == 0){
		ff = fopen(fname, "r");
	}
	else if (mode == 1) {
		ff = fopen(fname, "w");
	}
	else {
		//TODO: gestione errore parametro mode errato
	}
	if (ff == NULL){
		printf("Error opening file");
		//TODO: gestire errore, errno
	}
	bf->fd = fileno(ff);
	bf->mode = mode;
	bf->bufsize = bufsize;
	bf->n_bits = 0;
	//bf->ofs = 0;
	//bf->w_inizio = 0;
	bf->buf = (char*)malloc(bufsize);
	if (bf->buf == NULL) {
		//TODO: gestiore errori
	}
	bzero(bf->buf, bufsize);
	return bf;
}

//leggere n_bits da base e scriverli in fp->buf
int bit_write(struct bitfile* fp, const char* base, int n_bits, int ofs)
{
	//mask per leggere da base (1 sull'offset del bit da leggere)
	unsigned char mask = 1 << ofs;
	//mask per scrittura nel buffer di lavoro
	unsigned char w_mask = 1;
	char* p = base;
	unsigned int bit = 0;
	//byte del buffer di lavoro nel quale scrivere
	unsigned int pos = 0;
	unsigned int written_bits = 0;
	unsigned int flushed_bits = 0;
	while(n_bits > 0){
		bit = (*p & mask) ? 1 : 0;
		if (mask == 0x80){
			// si riparte dal primo bit del successivo byte
			p++;
			mask = 1;
		}
		else {
			mask = mask << 1;
		}
		//scrittura nel buffer fp->buf
		//devo scrivere il bit letto nel byte puntato da fp->buf, all'offset fp->ofs
		//n.b.: fp->ofs == fp->n_bits % 8 !
		//w_mask = w_mask << fp->ofs;
		w_mask = w_mask << (fp->n_bits % 8);
		pos = fp->n_bits / 8;
		//la scrittura qui è sempre possibile, il buffer pieno viene gestito dopo
		if (bit == 1){
			fp->buf[pos] |= w_mask;
		}
		if (bit == 0){
			fp->buf[pos] &= (!w_mask);
		}
		n_bits--;
		written_bits++;
		fp->n_bits++;
		//fp->ofs = (fp->ofs + 1) % 8;
		//test sul flush
		if (fp->n_bits == (fp->bufsize * 8)){
			flushed_bits = bit_flush(fp);

			//TODO: fare confronto dividendo per 8 entrambi?
			if (fp->n_bits != flushed_bits){
				printf("Write: not all bytes flushed");
			}
		}
	}
	return written_bits;
}

int bit_flush(struct bitfile* fp)
{
	FILE* ff;
	//bit scritti nel file
	int bit_to_file;

	if (fp->n_bits < 8){
		return 0;
	}
	//scrivere il contenuto di fp->buf nel file fino all'ultimo byte *intero*
	//TODO: usare write o fwrite?
	bit_to_file = write(fp->fd, (const void *)fp->buf, fp->n_bits / 8);
	if (bit_to_file == -1 || bit_to_file == 0){
		printf("Flush: error flushing bits to file");
		//TODO: gestione errore
	}
	if (bit_to_file != (fp->n_bits / 8)){
		printf("Flush: error: not all data flushed to file");
		//TODO: gestione errore (ma anche no qui)
	}
	//l'ultimo byte non scritto (if any), viene messo all'inizio del buffer.
	//Se n_bits è multiplo di 8 viene copiato inutilmente, ma n_bits va
	//comunque a 0 resettando il buffer
	fp->buf[0] = fp->buf[fp->n_bits / 8];
	fp->n_bits = fp->n_bits % 8;

	return bit_to_file * 8;
}
