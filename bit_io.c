#include "bit_io.h"


//TODO: fare refactor dei tipi e usare delle macro o dei typedef (es.: uint32)

struct bitfile* bit_open(const char* fname, int mode, int bufsize)
 {
	FILE* ff;
	struct bitfile* bf = (struct bitfile*)malloc(sizeof(struct bitfile));
	if (bf == NULL) {
		sys_err("bit_open: malloc error allocating bitfile structure");
	}
	if (mode == READ_MODE){
		ff = fopen(fname, "r");
		if (ff == NULL){
			user_err("bit_open: warning: input file doesn't exist");
		}
	}
	else if (mode == WRITE_MODE) {
		ff = fopen(fname, "w");
	}
	else {
		user_err("bit_open: unknown value for parameter mode");
	}
	if (ff == NULL){
		sys_err("bit_open: error opening file");
	}
	bf->fd = fileno(ff);
	bf->mode = mode;
	bf->bufsize = bufsize;
	bf->n_bits = 0;
	//bf->ofs = 0;
	//bf->w_inizio = 0;
	bf->buf = (char *)malloc(bufsize);
	if (bf->buf == NULL) {
		sys_err("bit_open: malloc error allocating buffer");
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
	char* p = (char *)base;
	unsigned int bit = 0;
	//byte del buffer di lavoro nel quale scrivere
	unsigned int pos = 0;
	unsigned int written_bits = 0;
	unsigned int flushed_bits = 0;
	unsigned int tmp = 0;

	while(n_bits > 0){
		bit = (*p & mask) ? 1 : 0;

//printf("Bit letto: %d\t Tot: %d\t Numero: %d\t\n", bit, fp->n_bits, written_bits + 1);

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
		//w_mask = w_mask << (fp->n_bits % 8);
		w_mask = 1 << (fp->n_bits % 8);
		pos = fp->n_bits / 8;
		//la scrittura qui è sempre possibile, il buffer pieno viene gestito dopo
		if (bit == 1){
			fp->buf[pos] |= w_mask;
		}
		if (bit == 0){
			fp->buf[pos] &= (~w_mask);
		}
		n_bits--;
		written_bits++;
		fp->n_bits++;
		//fp->ofs = (fp->ofs + 1) % 8;
		//test sul flush
		if (fp->n_bits == (fp->bufsize * 8)){
			tmp = fp->n_bits;
			flushed_bits = bit_flush(fp);
			if (tmp != flushed_bits){
				printf("Write: not all bytes flushed\n");
			}
		}
	}
//printf("Bit 0..7: %d\n", fp->buf[0]);
//printf("Bit 8..15: %d\n", fp->buf[1]);
//printf("Bit 16..23: %d\n", fp->buf[2]);
//printf("Bit 24..31: %d\n", fp->buf[3]);
	return written_bits;
}

int bit_flush(struct bitfile* fp)
{
	//bit scritti nel file
	int bit_to_file;

	if (fp->n_bits < 8){
		return 0;
	}
	//scrivere il contenuto di fp->buf nel file fino all'ultimo byte *intero*
	//TODO: usare write o fwrite? write
	bit_to_file = write(fp->fd, (const void *)fp->buf, fp->n_bits / 8);
	if (bit_to_file == -1 || bit_to_file == 0){
		sys_err("bit_flush: error flushing bits to file");
	}
	if (bit_to_file != (fp->n_bits / 8)){
		printf("bit_flush: note: not all data flushed to file\n");
	}
	//l'ultimo byte non scritto (if any), viene messo all'inizio del buffer.
	//Se n_bits è multiplo di 8 non deve essere fatto, ma n_bits va
	//comunque a 0 resettando il buffer
	if (fp->n_bits % 8 != 0){
		fp->buf[0] = fp->buf[fp->n_bits / 8];
	}

	fp->n_bits = fp->n_bits % 8;

	return bit_to_file * 8;
}

int bit_read(struct bitfile* fp, char* buf, int number, int ofs)
{
	//fp->n_bits è il numero di bit letti, per proseguire correttamente
	//con successive letture

	//per capire se si sono esauriti i bit letti, basta controllare se il numero
	//di bit letti è uguale a fp->bufsize * 8
	//Questo potrebbe non accadere mai se si usa un buffer più grande della
	//dimensione del file da leggere (fp->buf non si piena mai, e alla read
	//viene indicato di leggere più byte di quanti ce ne sono nel file

	int read_byte;
	char* p = &(fp->buf[fp->n_bits / 8]);
	unsigned int r_mask = 1 << (fp->n_bits % 8);
	unsigned int bit = 0;
	unsigned int w_mask = 1 << ofs;
	unsigned int read_bit = 0;

	while (number > 0) {
		if (fp->n_bits == fp->bufsize * 8 || fp->n_bits == 0) {
			//working buffer (fp->buf) empty, refill and restart reading
			//the user is demanded to check that buf is big enough!

			//buffer refill
			read_byte = read (fp->fd, (void *)fp->buf, fp->bufsize);
			if (read_byte == 0) {
				break;
			}
			if (read_byte == -1) {
				sys_err("bit_read: error reading from file");
			}
			if (read_byte < fp->bufsize) {
				printf("bit_read: note: working buffer only partially"
						" filled: %u bytes read\n", read_byte);
			}
			fp->n_bits = 0;
			p = fp->buf;
			r_mask = 1;
		}
		bit = (*p & r_mask) ? 1 : 0;
		if (r_mask == 0x80) {
			r_mask = 1;
			p++;
		}
		else {
			r_mask = r_mask << 1;
		}
		//buf always starts at bit 0 of byte 0
		if (bit == 1) {
			*buf |= w_mask;
		}
		else {
			*buf &= (~w_mask);
		}
		if (w_mask == 0x80) {
			w_mask = 1;
			buf++;
		}
		else {
			w_mask = w_mask << 1;
		}
		number--;
		fp->n_bits++;
		read_bit++;
	}
	return read_bit;
}

//Versione che legge sempre da 0, dove fp->n_bits rappresenta il numero di bit
//in fp->buf
int bit_read1(struct bitfile* fp, char* buf, int n_bits, int ofs)
{
	//leggo da file e riempio fp->buf
	//scrivo bit da fp->buf a buf
	//se fp->buf si svuota, lo ripieno

	unsigned int rbit_from_file = 0;
	char* p = fp->buf;
	unsigned char r_mask = 1;
	unsigned char w_mask = 1 << ofs;
	unsigned int bit = 0;
	unsigned int r_bits = 0;

	while (n_bits > 0){
		if (fp->n_bits == 0){
			//ricarica del buffer
			rbit_from_file = read(fp->fd, (void *)fp->buf, fp->bufsize);
			if (rbit_from_file == -1){
				sys_err("bit_read: error reading from file");
			}
			if (rbit_from_file == 0){
				//file finito
				break;
			}

			if (rbit_from_file < fp->bufsize){
				printf("bit_read: note: working buffer only partially"
						" filled: %u bytes read\n", rbit_from_file);
			}
			fp->n_bits = rbit_from_file * 8;
			p = fp->buf;
		}
		//fp->buf è sempre allineato al byte
		bit = (*p & r_mask) ? 1 : 0;
		if (r_mask == 0x80){
			//si è appena letto l'ultimo bit del byte, quindi si scorre
			p++;
			r_mask = 1;
		}
		else {
			r_mask = r_mask << 1;
		}

		//scrittura bit letto
		//w_mask = w_mask << ofs;
		if (bit == 1){
			*buf |= w_mask;
		}
		else {
			*buf &= (~w_mask);
		}
		if (w_mask == 0x80){
			w_mask = 1;
			//ofs = 0;
			buf++;
		}
		else {
			w_mask = w_mask << 1;
		}
		n_bits--;
		fp->n_bits--;
		r_bits++;
	}
	return r_bits;
}

int bit_close (struct bitfile* fp)
{
	//se il file è aperto in lettura, si chiude
	//se il file è aperto in scrittura, si fa flush e si scrivono
	//gli ultimi bit che non completano il byte
	//si devono anche liberare le zone di memoria allocate

	unsigned int cont = 0;
	unsigned int ris = 0;
	unsigned char mask = 1;

	if (fp->mode == 1){
		//la flush dovrebbe tornare 0 se è stata fatta prima della close
		cont = bit_flush(fp);
		//(fp->n_bits / 8)* 8): prendo un numero di byte interi e calcolo i bit contenuti
		//tralasciando ev. bit che non completano il byte
		if (cont != ((fp->n_bits / 8)* 8) ){
			user_err("bit_close: flushing error, use bit_flush before bit_close");
		}
		//scrittura su file dell'ultimo byte
		if (fp->n_bits != 0) {
			//si azzerano tutti i bit tranne quelli significativi
			mask = (1 << (fp->n_bits)) - 1;
			//fp->n_bits/8 dovrebbe essere 0
			//bit validi solo nel primo byte (perché è stata eseguita la flush)
			fp->buf[0] &= mask;

			ris = write(fp->fd, (const void *)fp->buf, 1);
			if (ris == -1 || ris == 0){
				user_err("bit_close: error flushing last rough bits");
			}
			cont += fp->n_bits;
		}
	}

	close(fp->fd);
	free(fp->buf);
	free(fp);
	return cont;
}
