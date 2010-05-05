#include "lz78.h"

//TODO: usare primo bit a 1 o 0 per evitare espansione
//TODO: configurare da utente la dimensione dei buffer per bitIO (dipende dalla)
//		dimensione del file
//TODO: free della tabella hash e del dizionario
//TODO (fatto): usare ceil_log2 con dict->d_next e valutare se eliminare hash size
//		(tanto quello che importa è avere un numero di bit sufficiente a
//		rappresentare la prossima codifica da usare)
//TODO (fatto): nella decode sequence non fare malloc ogni volta, ma estendere la lista
//		solo se il succ puntatore è NULL; fare la free solo alla fine.
//TODO: verificare comportamento bloccante/non bloccante (test con pipe)
//TODO: mettere contatore per le collisioni
//TODO: algoritmo per primo numero primo più piccolo di un numero dato

struct lz78_c* comp_init(){
	struct node* ht;
	struct lz78_c* dict;
	unsigned int size;
//	unsigned int i = 0;

	//DICT_SIZE ovvero 2^21
	size = DICT_SIZE;
	//9 byte * 2^21 = 18MB
	ht = (struct node*)malloc(size * sizeof(struct node));
	if (ht == NULL){
		sys_err("comp_init: error allocating hash table");
	}
	bzero(ht, size);

	dict = (struct lz78_c*)malloc(sizeof(struct lz78_c));
	if (dict == NULL){
		sys_err("comp_init: error allocating lz78_c struct");
	}
	bzero(dict, sizeof(struct lz78_c));

	dict->dict = ht;
	//Effective hash table size is 0, but we consider the presence of characters
	//from 0 to 255, which is implicit
	//TODO: questo è utile per partire subito con 9 bit, oppure è meglio
	//mettere hash_size a 0 e forzare la ceil_log2 a ritornare almeno 9?
	dict->hash_size = FIRST_CODE - 1;
	dict->d_next = FIRST_CODE;
	//dict->nbits = ceil_log2(dict->hash_size);
	dict->nbits = ceil_log2(dict->d_next);
	dict->cur_node = ROOT_CODE;
	return dict;
}


struct lz78_c* decomp_init()
{
	unsigned int size = DICT_SIZE;
	struct node* ht = NULL;
	struct lz78_c* decomp;

	ht = (struct node*)malloc(size * sizeof(struct node));
	if (ht == NULL){
		sys_err("decomp_init: error allocating hash table");
	}
	bzero(ht, size);

	decomp = (struct lz78_c*)malloc(sizeof(struct lz78_c));
	if (decomp == NULL){
		sys_err("decomp_init: error allocating lz78_c struct");
	}
	bzero(decomp, sizeof(struct lz78_c));

	decomp->cur_node = ROOT_CODE;
	decomp->d_next = FIRST_CODE;
	decomp->dict = ht;
	//TODO: stesse considerazioni che nella comp_init
	decomp->hash_size = FIRST_CODE - 1;
	decomp->nbits = ceil_log2(decomp->d_next);

	return decomp;
}

//unsigned int find_child_node(unsigned int parent_code,
//		unsigned int child_char,
//		struct lz78_c* comp)
//{
//	unsigned int index = 0;
//	while (index <= DICT_SIZE) {
//		if (comp->dict[index].code == EMPTY_NODE_CODE) {
//			return index;
//		}
//		if (comp->dict[index].parent_code == parent_code &&
//			comp->dict[index].character == (char)child_char) {
//			return index;
//		}
//		else {
//			//conflicts++;
//			index ++;
//		}
//	}
//	user_err ("Full dictionary!");
//	return 0;
//}


//from "The Data Compression Book"
unsigned int find_child_node(unsigned int parent_code,
		unsigned int child_char,
		struct lz78_c* comp)
{
	unsigned int index;
	int offset;
	//TODO: var di debug
	unsigned int conflicts = 0;

	/* Con lo shift al max si ottiene 255 << 13 = 2088960 (DICT_SIZE 2097143)
	 * Segue xor col codice del padre ==> sicuramente index è nel range
	 * (lo shift è fatto in modo da avere 21 bit: si parte da 8 e si
	 * shifta di 21 - 8!)
	 * */
	index = (child_char << ( BITS - 8 )) ^ parent_code;
	if (index == 0) {
		offset = 1;
	}
	else {
		offset = DICT_SIZE - index;
	}
	for ( ; ; ) {

		//TODO: if di debug
		if (conflicts > DICT_SIZE) {
			printf ("Conflitti: %u\n", conflicts);
			printf ("Hash size: %u\n\n", comp->hash_size);
		}

		//TODO: if di debug
		if (index > DICT_SIZE) {
			printf ("Index: %u\n", index);
			pause();
		}

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
			conflicts++;
			index -= offset;
		}
		else {
			conflicts++;
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
	int counter = 0;

	for (; ; ) {
		ch = fgetc(in);
		counter++;

		if (ch == EOF){
			//scrivere la codifica della sequenza corrente
			//scrivere la codifica di fine file
			bit_write(out, (const char*)(&(comp->cur_node)), comp->nbits, 0);
			comp->cur_node = EOF_CODE;
			bit_write(out, (const char*)(&(comp->cur_node)), comp->nbits, 0);
			bit_flush(out);
			bit_close(out);
			break;
		}

		if (comp->cur_node == ROOT_CODE) {
			//since it starts from root code, the next char read will be < 256
			comp->cur_node = ch;
			continue;
		}

		index = find_child_node(comp->cur_node, ch, comp);

		if (index > DICT_SIZE) {
			user_err ("find_child_node: index out of bound");
		}

		if (comp->dict[index].code == EMPTY_NODE_CODE){
			//the child doesn't exist
			comp->dict[index].character = (char)ch;
			comp->dict[index].code = comp->d_next;
			comp->dict[index].parent_code = comp->cur_node;
			//TODO: verificare ritorno
			ret = bit_write(out, (const char *)(&(comp->cur_node)),
					comp->nbits, 0);

			//next node will be the last not matching
			comp->cur_node = ch;
			comp->hash_size++;
			comp->d_next++;
			comp->nbits = ceil_log2(comp->hash_size);
<<<<<<< HEAD:lz78.c
			//comp->nbits = ceil_log2(comp->d_next);
			print_comp_ht(comp);
			continue;
=======
//			continue;
>>>>>>> 6f9838eafd284641273d1c8c687353ab4f4ec0d2:lz78.c
		}

		else if ( (comp->dict[index].character == (char)ch) &&
				(comp->dict[index].parent_code == comp->cur_node) ) {
			//a match
			comp->cur_node = comp->dict[index].code;
			continue;
		}
		else {
			user_err("lz78_compress: error in hash function");
		}

		if (comp->hash_size >= DICT_SIZE){
			//scrivere sul file compresso la codifica attuale
			//scrivere EOD sul file compresso
			//bzero(hash_table)
			//inizializzare i campi comp->... (stato del compressore)
<<<<<<< HEAD:lz78.c

printf("[DEBUG SEGMENTATION FAULT] in nuovo if\n");
=======
			printf ("New Dict\n");
>>>>>>> 6f9838eafd284641273d1c8c687353ab4f4ec0d2:lz78.c
			bit_write(out, (const char*)(&(comp->cur_node)), comp->nbits, 0);
			comp->cur_node = EOD_CODE;
			bit_write(out, (const char*)(&(comp->cur_node)), comp->nbits, 0);
			bzero(comp->dict, DICT_SIZE * sizeof(struct node) );
			comp->hash_size = FIRST_CODE - 1;
			comp->cur_node = ROOT_CODE;
			comp->d_next = FIRST_CODE;
			//TODO: formalmente hash_size, però non cambia
			comp->nbits = ceil_log2(comp->d_next);
		}
	}

	free(comp->dict);
	free(comp);
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
//	unsigned int i = 0;

	printf("Hash size: %u\n", comp->hash_size);
	printf("Bits used: %u\n", comp->nbits);
	printf("Current node: %u\n", comp->cur_node);
	printf("Next code to use: %u\n", comp->d_next);
//	for (i = 0; i < DICT_SIZE; i++) {
//		if (comp->dict[i].code != EMPTY_NODE_CODE) {
//			printf("Index: %u\t"
//					"Parent: %u\t"
//					"Code: %u\t"
//					"Label (int-casted): %d\t\n",
//					i,
//					comp->dict[i].parent_code,
//					comp->dict[i].code,
//					comp->dict[i].character);
//		}
//	}
}

/**
 * In the decompress function, the hash table is accessed directly using
 * the node code value.
 */
void lz78_decompress(struct lz78_c* decomp, FILE* out, struct bitfile* in)
{
	//legge una codifica lunga comp->nbits con bit_read, se l'intero
	//corrispondente è compreso fra 0 e 255 viene emesso direttamente
	//il carattere ASCII corrispondente.
	//Ogni volta che viene letta una codifica, viene preparata una entry
	//nella tabella hash, che ha come padre la codifica letta, come carattere
	//quello che verrà

	unsigned int read_code = 0;
	int ret = 0;
	struct seq_elem* sequence = NULL;
	char last_c = 0;

	//TODO: prima di leggere le codifiche dei caratteri, leggere eventuali
	//codici speciali, come lunghezza dizionario etc.

	//the first read code is ROOT's child, every other code, is at least
	//a single ASCII code child
	//so a new dict entry won't be created for the first code
	ret = bit_read(in, (char *)(&read_code), decomp->nbits, 0);
	while (ret < decomp->nbits) {
		printf ("lz78_decompress: caution, into the while!\n");
		ret += bit_read(in, (char *)(&read_code), (decomp->nbits - ret), ret);
	}

	decomp->cur_node = read_code;
	if (read_code == EOF_CODE) {
		bit_close(in);
		return;
	}

	//the first read code is between 0 and 255 and it is an ASCII code
	//TODO: controllare ritorno (scrittura carattere nel file)
	putc(read_code, out);

	for (; ;) {
		read_code = 0;
		ret = bit_read(in, (char *)(&read_code), decomp->nbits, 0);
		while (ret < decomp->nbits) {
			printf ("lz78_decompress: caution, into the while!\n");
			ret += bit_read(in, (char *)(&read_code),
					(decomp->nbits - ret), ret);
		}

		//TODO: fare controlli su read_code, es.: finefile
		if (read_code == EOF_CODE) {
			break;
		}

		if (read_code == EOD_CODE){
			bzero(decomp->dict, DICT_SIZE * sizeof(struct node) );
			decomp->hash_size = FIRST_CODE - 1;
			decomp->cur_node = ROOT_CODE;
			decomp->d_next = FIRST_CODE;
			decomp->nbits = ceil_log2(decomp->d_next);

			//TODO: codice ridondante, uguale a prima del ciclo
			//reading first code as initialization, it provides a parent code
			ret = bit_read(in, (char *)(&read_code), decomp->nbits, 0);
			while (ret < decomp->nbits) {
				printf ("lz78_decompress: caution, into the while!\n");
				ret += bit_read(in, (char *)(&read_code), (decomp->nbits - ret), ret);
			}
			decomp->cur_node = read_code;
			if (read_code == EOF_CODE) {
				bit_close(in);
				return;
			}
			putc(read_code, out);
			continue;
		}

		//TODO: if di debug
		if (read_code > decomp->d_next) {
			printf ("Cur node: %u\n", decomp->cur_node);
			printf ("Hash size: %u\n" ,decomp->hash_size);
			printf ("D_NEXT: %u\n", decomp->d_next);
			printf ("READ_CODE: %u\n", read_code);
			printf ("N_BITS: %u\n", decomp->nbits);
			printf ("/////////////////////////\n");
			pause();
		}

		//codice: successiva codifica da usare
		decomp->dict[decomp->d_next].code = decomp->d_next;
		//padre: nodo letto al passo precedente
		decomp->dict[decomp->d_next].parent_code = decomp->cur_node;

		/**
		 * Può succedere che la codifica appena creata sia subito da
		 * leggere per decodificare, in questo caso non si può ancora avere
		 * un carattere ad essa associata.
		 * Allora si prepara una entry senza carattere, si fa una prima
		 * decodifica per arrivare fino al carattere più vicino alla radice
		 * (è sempre quello che viene messo nel campo character), poi si
		 * riempie il campo carattere e si fa una decodifica completa.
		 */

		sequence = decode_sequence(decomp, read_code, sequence);

		//carattere: primo della sequenza (quello più vicino alla radice)
		decomp->dict[decomp->d_next].character = sequence->c;

		/*
		 * If the node corresponding to "read_code" was empty, the last char
		 * of the sequence hasn't been read, because it hasn't been written yet.
		 * So the last element of "sequence" won't be read and it is obtained
		 * directly from the hash table at "read_code".
		 * (This happens when read_code == decomp->d_next, but can be handled
		 * as a general case)
		 */

		last_c = (read_code < 256) ?
				(char) read_code : decomp->dict[read_code].character;

		//il padre del nodo successivo è il codice che è appena stato letto
		decomp->cur_node = read_code;
		decomp->d_next++;
		decomp->hash_size++;
		decomp->nbits = ceil_log2(decomp->d_next);

		while (sequence->prec != NULL) {
			putc(sequence->c, out);
			sequence = sequence->prec;
			//free(sequence->next);
		}
		putc(last_c, out);
		//free(sequence);
	}
	//here sequence points at the first element
	while (sequence->next != NULL) {
		sequence = sequence->next;
		free (sequence->prec);
	}
	free (sequence);
	free(decomp->dict);
	free(decomp);
	return;
}

struct seq_elem* decode_sequence(struct lz78_c* d, unsigned int code,
		struct seq_elem *sequence)
{
	if (sequence == NULL) {
		sequence = (struct seq_elem *)malloc(sizeof(struct seq_elem));
		bzero (sequence, sizeof(struct seq_elem));
		sequence->prec = NULL;
	}

	while (code >= FIRST_CODE) {
		if (sequence->next == NULL) {
			sequence->next = (struct seq_elem *)malloc(sizeof(struct seq_elem));
			bzero (sequence->next, sizeof(struct seq_elem));
		}
		sequence->c = d->dict[code].character;
		sequence->next->prec = sequence;
		sequence = sequence->next;
		code = d->dict[code].parent_code;
	}
	sequence->c = (char)code;
	//sequence->next = NULL; --> if it is the last elem,
							//it is null due to the bzero
	return sequence;


//	//char c;
//	//struct seq_elem* first = NULL;
//	struct seq_elem* seq = NULL;
//	seq = (struct seq_elem*)malloc(sizeof(struct seq_elem));
//	bzero (seq, sizeof(struct seq_elem));
//	seq->c = 0;
//	seq->prec = NULL;
//	seq->next = NULL;
//	//seq = first;
//
//	//se code < 255, d->dict[code].parent_code == 0 per l'inizializzazione della
//	//bzero()
//	//while(d->dict[code].parent_code >= FIRST_CODE) {
//	while(code >= FIRST_CODE) {
//
//		//TODO: if di debug, eliminare
//		if (d->dict[code].code == 0) {
//			printf ("ALEEEEEEEEEEEEEEERT!!!\n");
//			printf ("Code: %u\n", code);
//			printf ("Cur node: %u\n", d->cur_node);
//			printf ("Hash size: %u\n" ,d->hash_size);
//			printf ("D_NEXT: %u\n", d->d_next);
//			printf ("Parent code: %u\n", d->dict[code].parent_code);
//			printf ("/////////////////////////\n");
//			pause();
//		}
//
//		seq->c = d->dict[code].character;
//		code = d->dict[code].parent_code;
//		seq->next = (struct seq_elem*)malloc(sizeof(struct seq_elem));
//		bzero (seq->next, sizeof(struct seq_elem));
//		seq->next->prec = seq;
//		seq = seq->next;
//	}
//	seq->c = (char)code;
//	seq->next = NULL;
//	//leggendo i caratteri dall'elemento puntato da sec, finché
//	//seq->prec != NULL, si ricostruisce la codifica
//	return seq;
}
