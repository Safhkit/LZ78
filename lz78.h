#include "bit_io.h"
#include "utility.h"
#include <string.h>
#include "stack.h"

#ifndef __LZ78_H__
#define __LZ78_H__

/**
 * Si suppone che nel dizionario siano già presenti le codifiche da 0 a 255, che
 * rappresentano il carattere ASCII corrispondente.
 * Queste entry però non vengono aggiunte nella tabella hash, si ricavano
 * implicitamente.
 * Questo consente di usare le codifiche da 0 a 255 per scopi diversi rispetto
 * alla rappresentazione delle codifiche dei caratteri, ammesso che i codici
 * partano da 256 (il numero minimo di bit per rappresentare le codifiche è 9).
 * Quindi nella tabella non ci potrà mai essere nessuna codifica da 0 a 255, se
 * non quelle speciali.
 * Nel file compresso invece ci saranno valori da 0 a 255, per rappresentare
 * le sequenze di singoli caratteri, quindi valori speciali da passare al
 * decompressore (ovvero che dovranno essere scritti nel file compresso),
 * dovranno essere maggiori di 255.
 * I valori da 255 a FIRST_CODE sono speciali e scrivibili nel file compresso.
 * I valori speciali da 0 a 255 non possono essere scritti nel file compresso.
 * Nessun nodo può avere codice da 0 a 255, però cur_node può valere da 0 a 255
 * quando si è appena letto un carattere singolo. Quindi ROOT_CODE deve essere
 * maggiore di 255 (per come viene usato, infatti viene confrontato
 * con cur_node).
 */

//DICT_SIZE: prime number close to 2 * 2^20 = 2097152
//TODO: la dimensione del dizionario deve poter essere specificata dall'utente
//come calcolare numero primo + piccolo della dim passata?
//2097143
#define DICT_SIZE 2097169

//max number of bit for codes
#define BITS 21

#define MAX_SEQUENCE_LENGTH ((DICT_SIZE >> 8) + 1)

//root node's code, used as cur_node value to notify the beginning of a new
//sequence
#define ROOT_CODE 256

//code for empty nodes, using 0 allows to initialize the hash table with bzero()
#define EMPTY_NODE_CODE 0

//comunicazione al decompressore della dimensione del dizionario
//#define DICT_LENGTH_CODE

//comunicazione di fine file
#define EOF_CODE 257

//end of dictionary code
#define EOD_CODE 258

//first available code when the hash table is created
#define FIRST_CODE 259

struct node {
	//from FIRST_CODE to 2^21
	unsigned int code;
	unsigned char character;
	//from 0 to 2^21
	unsigned int parent_code;
};

struct seq_elem {
	unsigned char c;
	struct seq_elem* next;
	struct seq_elem* prec;
	//unsigned int count;
};

struct codes_queue {
	unsigned int code;
	unsigned char c;
	struct codes_queue *next;
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
	 * Next code to use when inserting a new node in the hash table
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

struct lz78_c* comp_init();
struct lz78_c* decomp_init();
unsigned int find_child_node(unsigned int parent_code,
		unsigned int child_char,
		struct lz78_c* comp);

void lz78_compress(struct lz78_c* c, FILE* in, struct bitfile* out);

void lz78_decompress(struct lz78_c* c, FILE* out, struct bitfile* in);

struct seq_elem* decode_sequence(struct lz78_c* d,
		unsigned int code,
		struct seq_elem *s);

/**
 * Utility function which calculates the ceiling of a base 2 log of an integer
 */
unsigned int ceil_log2(unsigned int x);

struct codes_queue *string_to_code1(struct lz78_c* comp,
		struct seq_elem *seq,
		struct codes_queue *q);

struct codes_queue *insert_queue (unsigned int code, unsigned char c, struct codes_queue *q);

unsigned int read_next_code(struct bitfile *in, unsigned int n_bits);

void decode_stack (struct d_stack *s, struct lz78_c *d, unsigned int code);

void lz78_destroy(struct lz78_c *s);

unsigned int string_to_code (struct d_stack *s, struct lz78_c *comp);

unsigned char root_char (unsigned int code, struct lz78_c *c);

//TODO: definire solamente con flag debug
void print_comp_ht(struct lz78_c* comp);


#endif

