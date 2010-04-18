#include "lz78.h"

struct node* dict_init(){
	struct node* dictionary;
	unsigned int double_size;
	int i = 0;

	//2*DICT_SIZE, ovvero 2^21
	double_size = DICT_SIZE << 1;
	//9 byte * 2^21 = 18MB
	dictionary = (struct node*)malloc(double_size * sizeof(struct node));
	if (dictionary == NULL){
		sys_err("dict_init: error allocating dictionary");
	}
	bzero(dictionary, double_size);
	for (i = 0; i < 256; i++) {
		dictionary[i].character = (char)i;
		dictionary[i].code = i;
		dictionary[i].parent_code = 0;
	}
	for (; i < double_size; i++) {
		dictionary[i].character = 0;
		dictionary[i].code = -1;
		dictionary[i].parent_code = 0;
	}

	return dictionary;
}
