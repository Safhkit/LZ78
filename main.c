#include <stdio.h>
#include <stdlib.h>
#include "bit_io.h"
#include "lz78.h"

#define BIT_IO_BUFFER_SIZE 40000

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
	int size_not_set = 1;

	if (argc < 4){
		printf("Too few arguments!\n");
		Usage();
	}

	verbose_mode = 0;
	while ( (opt = getopt (argc, argv, "c:d:ao:b:v")) != -1 ) {
		switch (opt)
		{
		case 'c':
			if (c_vs_d != 100)
				Usage();
			else {
				ifile = optarg;
				c_vs_d = 0;
			}
			break;
		case 'd':
			if (c_vs_d != 100)
				Usage();
			else {
				ifile = optarg;
				c_vs_d = 1;
			}
			break;
		case 'a':
			antiexpflag = 1;
			break;
		case 'o':
			ofile = optarg;
			break;
		case 'b':
			set_size(atoi(optarg));
			size_not_set = 0;
			break;
		case 'v':
			verbose_mode = 1;
			break;
		default:
			Usage();
			break;
		}
	}
	if (ofile == NULL || c_vs_d == 100)
		Usage();
	if (c_vs_d == 0) {
		//compression
		if (size_not_set)
			set_size(21);
		printf ("Compressing with %u bits as maximum code length\n", BITS);
		printf ("Compressing with dict_size: %u\n", DICT_SIZE);
		compress_file(ifile, ofile, antiexpflag);
	}
	else {
		if (!size_not_set) {
			printf ("Cannot specify dictionary size in decompression\n");
			Usage();
		}
		decompress_file(ifile, ofile);
	}

	return 0;
}

void Usage()
{
	printf("Usage:\n"
			"\tlz78 -c  <input_file> -o <output_file> [-a][-b <bits>]\n"
			"\tlz78 -d  <input_file> -o <output_file> [-a]\n\n");
	printf("\t-c\tcompress\n"
			"\t-d\tdecompress\n"
			"\t-o\tspecify the output file\n"
			"\t-a:\tanti expansion check\n"
			"\t-b:\tmaximum code length, it influences dictionary length which\n"
			"\t\twill be approximately 2^bits. Correct values from 10 to 21\n"
			"\t-v:\tverbose mode"
			"\n");
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
	compressor = lz78c_init();
	lz78_compress(compressor, infile, outfile, aexpand);
//	print_comp_ht(compressor);
	fclose(infile);

	return;
}

void decompress_file(char* fname, char* fdecompressed) {
	struct bitfile* infile;
	struct lz78_c* decompressor;
	FILE* outfile;
	uint8_t bits = 0;

	infile = bit_open(fname, READ_MODE, BIT_IO_BUFFER_SIZE);
	outfile = fopen(fdecompressed, "w");
	bit_read(infile, (char *)(&bits), 8, 0);
	set_size(bits);
	printf ("Decompressing with %u bit as maximum code length\n", BITS);
	printf ("Decompressing with dict_size: %u\n", DICT_SIZE);
	decompressor = lz78c_init();
	lz78_decompress(decompressor, outfile, infile);
//	print_comp_ht(decompressor);
	fclose(outfile);
	return;
}
