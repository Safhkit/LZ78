#include "bit_io.h"


//TODO: free di bf

 bitfile* bit_open(const char* fname, int mode, int bufsize)
 {
	FILE* ff;
	bitfile* bf = (bitfile*)malloc(sizeof(bitfile));
	if (mode == 0){
		ff = fopen(fname, "r");
	}
	else {
		ff = fopen(fname, "w");
	}
	if (ff == NULL){
		printf("Error opening file");
		//TODO: gestire errore, errno
		
		return NULL;
	}
	bf->fd = fileno(ff);
	bf->mode = mode;
	bf->bufsize = bufsize;
	bf->n_bits = 0;
	bf->ofs = 0;
	bf->w_inizio = 0;
	
	/* +1 per poter mettere '\0', così nelle altre funzioni è possibile fare
	   controllo se n_bits > bufsize con la strlen(), che smette di contare quando
	   raggiunge '\0' e non lo conta */
	bf->buf = (char*)malloc(bufsize+1);
	bzero(bf->buf);
	bf->buf[bufsize-1] = '\0';
	return bf;
}


int bit_write(bitfile* fp, const char* base, int n_bits, int ofs)
{
	//leggere n_bits da base e scriverli in fp->buf
	
	//1 sull'offset del byte da leggere
	unsigned char mask = 1 << ofs;
	char* p = base;
	const int n_bits_max = n_bits;
	int i=0;
	while(n_bits > 0){
		//scrittura nel buffer fp->buf
		fp->buf[i] |= (*p & mask) ? (1 << (n_bits_max - n_bits)) ? 0;
		if (!(n_bits_max - n_bits)) % 8) i++;
		
		if (mask == 0x80){
			// si riparte dal primo bit del successivo byte
			p++;
			mask = 1;
		}
		else {
			mask = mask << 1;
		}
		
		n_bits--;

		if ((n_bits_max - n_bits) == strlen(fp->buf)){
			//abbiamo scritto n_bits nel buffer di lavoro o questo è pieno
			bit_flush(fp);
			i=0;
		}
	}

	return n_bits;	//successo
}


int bit_read(struct bitfile* fp, char* buf, int n_bits, int ofs)
{
	
}

