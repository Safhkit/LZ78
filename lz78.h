#include "bit_io.h"
#include "utility.h"

//DICT_SIZE: 2^20
#define DICT_SIZE (1 << 20)

struct node {
	int code;
	char character;
	int parent_code;
};

struct node* dict_init();
