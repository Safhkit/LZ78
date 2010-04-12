#include <stdio.h>
#include <stdlib.h>
#include "bit_io.h"

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
