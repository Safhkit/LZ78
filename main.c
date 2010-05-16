#include <stdio.h>
#include <stdlib.h>
#include "bit_io.h"
//#include "utility.h"
#include "lz78.h"

#define BIT_IO_BUFFER_SIZE 10000000

void compress_file(char* fname, char* fcompressed, int aexpand);
void decompress_file(char* fname, char* fdecompressed);
void Usage();

int main(int argc, char* argv[])
{
	int opt;
	int antiexpflag = 0;
	char *ifile = NULL;
	char *ofile = NULL;
	//compression 0, decompression 1
	int c_vs_d = 100;

//	unsigned char c = 0;
//	int i = 0;
//	for (i = 0; i < 256; i++) {
//		printf ("Carattere: %c\t Intero: %i\t Unsigned: %u\n", c, (int)c, (unsigned int)c);
//		printf ("Carattere: %c\t U_char: %c\t\n\n", (char)i, (unsigned char)i);
//		c++;
//	}
//	pause();
	if (argc < 4){
		printf("Too few arguments!\n");
		Usage();
	}

	while ( (opt = getopt (argc, argv, "c:d:ao:")) != -1 ) {
		switch (opt)
		{
		case 'c':
			if (c_vs_d != 100) {
				Usage();
				exit (0);
			}
			else {
				ifile = optarg;
				c_vs_d = 0;
//				compress_file(optarg, argv[3]);
			}
			break;
		case 'd':
			if (c_vs_d != 100) {
				Usage();
				exit (0);
			}
			else {
				ifile = optarg;
				c_vs_d = 1;
//				decompress_file(optarg, argv[3]);
			}
			break;
		case 'a':
			antiexpflag = 1;
			break;
		case 'o':
			ofile = optarg;
			break;
		default:
			Usage();
		}
	}
	if (ofile == NULL || c_vs_d == 100) {
		Usage();
		exit (0);
	}
	if (c_vs_d == 0) {
		//compression
		compress_file(ifile, ofile, antiexpflag);
	}
	else {
		decompress_file(ifile, ofile);
	}

	return 0;
}

void Usage()
{
	printf("Usage:\n"
			"\tlz78 -c  <input_file> -o <output_file> [-a]\n"
			"\tlz78 -d  <input_file> -o <output_file> [-a]\n\n");
	printf("\t-c\tcompress\n"
			"\t-d\tdecompress\n"
			"\t-o\tspecify the output file\n"
			"\t-a:\tanti expansion check\n");
	exit (0);
}

void compress_file(char* fname, char* fcompressed, int aexpand)
{
	struct bitfile* outfile;
	struct lz78_c* compressor;
	FILE* infile;

	infile = fopen(fname, "r");
	if (infile == NULL){
		user_err("Warning, the file to be compressed doesn't exist!");
	}
	outfile = bit_open(fcompressed, WRITE_MODE, BIT_IO_BUFFER_SIZE);
	compressor = comp_init();
	lz78_compress(compressor, infile, outfile, aexpand);
//	print_comp_ht(compressor);
	fclose(infile);

	return;
}

void decompress_file(char* fname, char* fdecompressed) {
	struct bitfile* infile;
	struct lz78_c* decompressor;
	FILE* outfile;

	infile = bit_open(fname, READ_MODE, BIT_IO_BUFFER_SIZE);
	outfile = fopen(fdecompressed, "w");
	decompressor = decomp_init();
	lz78_decompress(decompressor, outfile, infile);
//	print_comp_ht(decompressor);
	fclose(outfile);
	return;
}


/*
//test memorizzazione e riferimento ad interi
int main()
{
	int x = 344; //00000000 00000000 00000001 01011000
	char *p = NULL;

	p = (char *)&x;
	printf("Primo byte: %u\n", *(p++)); //88 @ 3221213516
	printf("Secondo byte: %u\n", *(p++)); //1 @ 3221213517
	printf("Terzo byte: %u\n", *(p++)); //0 @ 3221213518
	printf("Quarto byte: %u\n", *(p)); //0 @ 3221213519

	return 0;
}
*/

/*
//test della copia bit a bit di un file di dimensioni test_dim
int main(){
	struct bitfile* br;
	struct bitfile* bw;
	int bit_letti = 0;
	int bit_scritti = 0;
	int test_dim = 369093;

	char buf[test_dim];

	br = bit_open("test", 0, test_dim);
	bw = bit_open("dest", 1, test_dim);

	bit_letti = bit_read(br, buf, test_dim * 8, 0);
	printf("[main] bit letti: %d\n", bit_letti);

	bit_scritti = bit_write(bw, buf, test_dim * 8, 0);
	printf("[main] bit scritti: %d\n", bit_scritti);

	//bit_scritti = bit_flush(bw);
	//printf("[main] bit flushed: %d\n", bit_scritti);

	bit_close(bw);
	bit_close(br);

	return 0;
}
*/

/*
//test dello shift circolare
int main(){
	unsigned char p;
	int i = 0;
	for (i = 0; i < 300; i++){
		printf("Valore: %d\n", p);
		p = 1<<(i%8);
	}
}
*/

/*
//test della lettura bit (OK)
int main(){
	unsigned int num = 57596; //00000000000000001110000011111100
	char *p = (char*)&num;
	int n_bits = 32;
	int bit = 0;
	unsigned char mask = 1;
	while(n_bits > 0){
		bit = (*p & mask) ? 1 : 0;
		printf("%d", bit);
		if (mask == 0x80){
			// si riparte dal primo bit del successivo byte
			p++;
			mask = 1;
		}
		else {
			mask = mask << 1;
		}
		n_bits--;
	}
	printf("\n");
}
*/
