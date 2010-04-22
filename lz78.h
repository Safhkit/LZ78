#include "bit_io.h"
#include "utility.h"

#ifndef __LZ78_H__
#define __LZ78_H__

//DICT_SIZE: prime number close to 2 * 2^20 = 2097152
//TODO: la dimensione del dizionario deve poter essere specificata dall'utente
//come calcolare numero primo + piccolo della dim passata?
#define DICT_SIZE 2097143

//max number of bit for codes
#define BITS 21

//root node's code
#define ROOT_CODE 256

//code value for an empty node
#define EMPTY_NODE_CODE 257

//comunicazione al decompressore della dimensione del dizionario
//#define DICT_LENGTH_CODE
//comunicazione di fine file
//#define EOF_CODE

//first empty node when the hash table is first created
#define FIRST_CODE 258

struct node {
	unsigned int code;
	char character;
	unsigned int parent_code;
};

struct lz78_c {
	struct node* dict;
/*
 * nodo corrente fino al quale si ha avuto match nel cercare una stringa
 * nell'albero. E' ciò che viene ritornato dalla find_child_node, e la codifica
 * da emettere sarà dict[cur_node].code (in bit). N.B.: la find child node deve
 * essere richiamata ogni volta, per vedere se il nodo corrente ha un figlio
 * con un certo carattere e quindi per prolungare il più possibile il match.
 *
 * Nodo ritornato dalla find_child_node, che viene chiamata per cercare se
 * un certo nodo (padre), ha un figlio con un certo carattere.
 * La find_child_node quindi deve essere chiamata più volte, finché si continua
 * a trovare match della sequenza. Quando ritorna un nodo vuoto, si emette
 * la codifica del nodo precedente, che corrisponde alla codifica della
 * sequenza che ha prodotto il match più lungo.
 * */
	unsigned int cur_node;
	/*
	 * Next location where to insert a new node in the hash table
	 * */
	unsigned int d_next;

	/**
	 * Hash table size
	 */
	unsigned int hash_size;

	/**
	 * Numero di bit delle codifiche, deve valere ceil(log2(hash_size))
	 */
	unsigned int nbits;
};

struct node* dict_init();
unsigned int find_child_node(unsigned int parent_code,
		unsigned int child_char,
		struct lz78_c* comp);

/**
 * Utility function which calculates the ceiling of a base 2 log of an integer
 */
unsigned int ceil_log2(unsigned int x);

#endif

