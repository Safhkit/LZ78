#include "bit_io.h"


struct bitfile* bit_open(const char* fname, uint8_t mode, uint32_t bufsize)
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
	bf->buf = (char *)malloc(bufsize);
	if (bf->buf == NULL) {
		sys_err("bit_open: malloc error allocating buffer");
	}
	bzero(bf->buf, bufsize);
	return bf;
}


uint32_t bit_write(struct bitfile* fp, const char* base, uint32_t n_bits, int ofs)
{
	//mask to read from base
	unsigned char mask = 1 << ofs;
	//mask to write to fp->buf
	unsigned char w_mask = 1;
	char* p = (char *)base;
	uint8_t bit = 0;
	uint32_t pos = 0;
	uint32_t written_bits = 0;
	uint32_t flushed_bits = 0;
	uint32_t tmp = 0;

	while(n_bits > 0){
		bit = (*p & mask) ? 1 : 0;

		if (mask == 0x80){
			// next bit of next byte
			p++;
			mask = 1;
		}
		else {
			mask = mask << 1;
		}
		//offset is fp->n_bits % 8
		w_mask = 1 << (fp->n_bits % 8);
		pos = fp->n_bits / 8;
		if (bit == 1){
			fp->buf[pos] |= w_mask;
		}
		if (bit == 0){
			fp->buf[pos] &= (~w_mask);
		}
		n_bits--;
		written_bits++;
		fp->n_bits++;

		//flush test
		if (fp->n_bits == (fp->bufsize * 8)){
			tmp = fp->n_bits;
			flushed_bits = bit_flush(fp);
			if (tmp != flushed_bits){
				printf("Write: not all bytes flushed\n");
			}
		}
	}

	return written_bits;
}

uint32_t bit_flush(struct bitfile* fp)
{
	//bits flushed to file
	int bit_to_file;

	if (fp->n_bits < 8){
		return 0;
	}
	//writes until last full byte
	bit_to_file = write(fp->fd, (const void *)fp->buf, fp->n_bits / 8);
	if (bit_to_file == -1 || bit_to_file == 0){
		sys_err("bit_flush: error flushing bits to file");
	}
	if (bit_to_file != (fp->n_bits / 8)){
		printf("bit_flush: note: not all data flushed to file\n");
	}
	//last not-full byte becomes the first one, if any, otherwise fp->n_bits
	//will be zero, keeping the structure consistent
	if (fp->n_bits % 8 != 0){
		fp->buf[0] = fp->buf[fp->n_bits / 8];
	}

	fp->n_bits = fp->n_bits % 8;

	return bit_to_file * 8;
}

uint32_t bit_read(struct bitfile* fp, char* buf, uint32_t number, int ofs)
{
	int read_byte;
	char* p = &(fp->buf[fp->n_bits / 8]);
	unsigned int r_mask = 1 << (fp->n_bits % 8);
	uint8_t bit = 0;
	unsigned int w_mask = 1 << ofs;
	uint32_t read_bit = 0;

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
//				printf("bit_read: note: working buffer only partially"
//						" filled: %u bytes read\n", read_byte);
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

int bit_close (struct bitfile* fp)
{
	uint32_t cont = 0;
	unsigned int ris = 0;
	unsigned char mask = 1;

	if (fp->mode == WRITE_MODE){
		//cont should be 0 if bit_flush was called before bit_close
		cont = bit_flush(fp);
		//both operands should be zero
		if (cont != ((fp->n_bits / 8)* 8) ){
		  close(fp->fd);
		  free(fp->buf);
		  free(fp);
		  user_err("bit_close: flushing error, use bit_flush before bit_close");
		}
		//writing last byte
		if (fp->n_bits != 0) {
			//reset all unused bits of this last byte
			mask = (1 << (fp->n_bits)) - 1;
			fp->buf[0] &= mask;

			ris = write(fp->fd, (const void *)fp->buf, 1);
			if (ris == -1 || ris == 0){
				close(fp->fd);
				free(fp->buf);
				free(fp);
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
