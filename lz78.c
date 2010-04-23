#include "lz78.h"

struct lz78_c* dict_init(){
	struct node* ht;
	struct lz78_c* dict;
	unsigned int size;
	unsigned int i = 0;
	unsigned int index = 0;

	//DICT_SIZE ovvero 2^21
	size = DICT_SIZE;
	//9 byte * 2^21 = 18MB
	ht = (struct node*)malloc(size * sizeof(struct node));
	if (ht == NULL){
		sys_err("dict_init: error allocating hash table");
	}
	bzero(ht, size);

	dict = (struct lz78_c*)malloc(sizeof(struct lz78_c));
	if (dict == NULL){
		sys_err("dict_init: error allocating lz78_c struct");
	}
	bzero(dict, sizeof(struct lz78_c));

	//TODO: eliminare le codifiche singole dal dizionario
	for (i = 0; i < 256; i++) {
		ht[i].character = (char)i;
		ht[i].code = i;
		ht[i].parent_code = ROOT_CODE;
	}
	//inefficiente: si dovrebbe sfruttare la bzero per inizializzare
	/*
	 * TODO: improvement: eliminare dalla hash le codifiche dei caratteri
	 * singoli (codifiche ASCII < 256): quando vengono letti, si conosce
	 * implicitamente che parent_code = 0, characheter = c, code = int(c).
	 * Di conseguenza devono essere cambiate le define: EMPTY_NODE 0, per avere
	 * inizializzazione fatta solo con la bzero, ROOT_CODE 1, etc.
	 * */
	for (; i < size; i++) {
		ht[i].character = 0;
		ht[i].code = EMPTY_NODE_CODE;
		ht[i].parent_code = 0;
	}
	dict->dict = ht;
	dict->hash_size = FIRST_CODE - 1;
	dict->d_next = FIRST_CODE;
	dict->nbits = ceil_log2(dict->hash_size);
	return dict;
}

//funzione ripresa dal "The Data Compression Book"
unsigned int find_child_node(unsigned int parent_code,
		unsigned int child_char,
		struct lz78_c* comp)
{
	unsigned int index;
	int offset;

	/* Con lo shift al max si ottiene 255 << 13 = 2088960 (DICT_SIZE 2097143)
	 * Segue xor col codice del padre ==> sicuramente index è nel range
	 * */
	index = (child_char << ( BITS - 8 )) ^ parent_code;
	if (index == 0) {
		offset = 1;
	}
	else {
		offset = DICT_SIZE - index;
	}
	for ( ; ; ) {
		if (comp->dict[index].code == EMPTY_NODE_CODE) {
			//empty node
			return((unsigned int)index);
		}
		if (comp->dict[index].parent_code == parent_code &&
				comp->dict[index].character == (char)child_char) {
			//match
			return(index);
		}
		if (index >= offset) {
			index -= offset;
		}
		else {
			index += DICT_SIZE - offset;
		}
	}
}

void lz78_compress(struct lz78_c* comp, FILE* in, struct bitfile* out)
{
	//legge carattere da file
	//per ogni carattere letto controlla se è figlio di un nodo già esistente
	//determina la codifica della sequenza
	//scrive su file nbits della codifica della sequenza
	int ch;
	unsigned int index;

	for (; ; ) {
		ch = fgetc(in);
		if (ch == EOF){
			//TODO: gestire fine file: usare codice di fine file
		}
		if (comp->cur_node == 0) {
			//si sta partendo dalla radice, lettura di un carattere
			//presente implicitamente nel dizionario, si inizia quindi
			//una nuova stringa che parte con questo singolo carattere
			comp->cur_node = ch;
		}

		index = find_child_node(c->cur_node, ch, comp);
		if (comp->dict[index].code == EMPTY_NODE_CODE){
			//figlio non esistente: inserire nuovo nodo nella tabella
			comp->dict[index].character = (char)ch;
			comp->dict[index].code = comp->d_next;
			comp->dict[index].parent_code = comp->cur_node;
			comp->cur_node = 0; //si riparte dalla radice
			comp->hash_size++;
			comp->d_next++;
			comp->nbits = ceil_log2(comp->hash_size);

		}
	}
}

unsigned int ceil_log2(unsigned int x)
{
	unsigned int cont = 0;

	if (x == 0) {
		user_err("ceil_log2: cannot calculate log(0)");
	}
	while (x > 0) {
		x = x >> 1;
		cont++;
	}
	return cont;
}
