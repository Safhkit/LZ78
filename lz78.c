#include "lz78.h"

//TODO: free della tabella hash e del dizionario

struct lz78_c* dict_init(){
	struct node* ht;
	struct lz78_c* dict;
	unsigned int size;
//	unsigned int i = 0;

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

//	//TODO: eliminare le codifiche singole dal dizionario
//	for (i = 0; i < 256; i++) {
//		ht[i].character = (char)i;
//		ht[i].code = i;
//		ht[i].parent_code = ROOT_CODE;
//	}
//	//inefficiente: si dovrebbe sfruttare la bzero per inizializzare
//	/*
//	 * TODO: improvement: eliminare dalla hash le codifiche dei caratteri
//	 * singoli (codifiche ASCII < 256): quando vengono letti, si conosce
//	 * implicitamente che parent_code = 0, characheter = c, code = int(c).
//	 * Di conseguenza devono essere cambiate le define: EMPTY_NODE 0, per avere
//	 * inizializzazione fatta solo con la bzero, ROOT_CODE 1, etc.
//	 * */
//	for (; i < size; i++) {
//		ht[i].character = 0;
//		ht[i].code = EMPTY_NODE_CODE;
//		ht[i].parent_code = 0;
//	}

	dict->dict = ht;
	//Effective hash table size is 0, but we consider the presence of characters
	//from 0 to 255, which is implicit
	dict->hash_size = FIRST_CODE - 1;
	dict->d_next = FIRST_CODE;
	dict->nbits = ceil_log2(dict->hash_size);
	dict->cur_node = ROOT_CODE;
	return dict;
}

//from "The Data Compression Book"
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
	int ret = 0;

	for (; ; ) {
		ch = fgetc(in);
		if (ch == EOF){
			//TODO: gestire fine file: usare codice di fine file
			//scrivere la codifica della sequenza corrente
			//scrivere la codifica di fine file
			bit_write(out, (const char*)(&(comp->cur_node)), comp->nbits, 0);
			comp->cur_node = EOF_CODE;
			bit_write(out, (const char*)(&(comp->cur_node)), comp->nbits, 0);
			//TODO: verificare se serve la bit_flush qui
			bit_flush(out);
			bit_close(out);
			return;
		}
		if (comp->cur_node == ROOT_CODE) {
			//si sta partendo dalla radice, lettura di un carattere
			//presente implicitamente nel dizionario, si inizia quindi
			//una nuova stringa che parte con questo singolo carattere
			comp->cur_node = ch;
			continue;
		}
		index = find_child_node(comp->cur_node, ch, comp);
		if (comp->dict[index].code == EMPTY_NODE_CODE){
			//figlio non esistente: inserire nuovo nodo nella tabella
			comp->dict[index].character = (char)ch;
			comp->dict[index].code = comp->d_next;
			comp->dict[index].parent_code = comp->cur_node;

			//emettere la codifica della sequenza letta, ovvero cur_node (il
			//codice del nodo precedente al quale si è interrotto il matching)
			//TODO: verificare ritorno
			ret = bit_write(out, (const char *)(&(comp->cur_node)),
					comp->nbits, 0);

//printf("Ret: %d\t nbits: %d\t ch: %d\t\n", ret, comp->nbits, ch);

			//si deve ripartire dall'ultimo carattere che non ha matchato
			//alcuna sequenza
			//TODO: verificare che fgetc non torni mai > 255
			comp->cur_node = ch; //si riparte dall'ultimo car non matchante
			comp->hash_size++;
			comp->d_next++;
			comp->nbits = ceil_log2(comp->hash_size);
			continue;
		}
		else if ( (comp->dict[index].character == (char)ch) &&
				(comp->dict[index].parent_code == comp->cur_node) ) {
			//cur_node deve prendere il codice del nodo all'indice trovato

//printf ("Sequenza\n");

			comp->cur_node = comp->dict[index].code;
			continue;
		}
		else {
			user_err("lz78_compress: error in hash searching function");
		}
		//TODO: controllare che la tabella hash non sia piena
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

	//TODO: rimuovere questo if
	if (cont < 8) {
		printf ("ceil_log2: cont minore di 8! Valore: %d", cont);
	}
	return cont;
}

//TODO: definire solamente con flag debug
void print_comp_ht(struct lz78_c* comp)
{
	unsigned int i = 0;

	printf("Hash size: %u\n", comp->hash_size);
	printf("Bits used: %u\n", comp->nbits);
	printf("Current node: %u\n", comp->cur_node);
	printf("Next code to use: %u\n", comp->d_next);
	for (i = 0; i < DICT_SIZE; i++) {
		if (comp->dict[i].code != EMPTY_NODE_CODE) {
			printf("Index: %u\t"
					"Parent: %u\t"
					"Code: %u\t"
					"Label (int-casted): %d\t\n",
					i,
					comp->dict[i].parent_code,
					comp->dict[i].code,
					comp->dict[i].character);
		}
	}
}
