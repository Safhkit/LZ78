#include "lz78.h"

//TODO: se utente mette BITS > 32 (ovvero DICT_SIZE > 2^32 - 1), le codifiche
//		non stanno più sugli interi e non funziona + niente. Fare controllo
//		sulla dim max del dizionario che è 2^32 - 1
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
//TODO: errori quando utente specifica la dimensione di BITS e DICT_SIZE

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

//TODO: è uguale alla comp_init()?
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
			pause();
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
				comp->dict[index].character == (unsigned char)child_char) {
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

//TODO: var di debug, rimuovere
unsigned int ultima = 0;
unsigned int ultima_d = 0;
int flag = 0;

void lz78_compress(struct lz78_c* comp, FILE* in, struct bitfile* out)
{
	int ch = 0;
	struct lz78_c *new_comp = NULL;

	for (;;) {
		ch = fgetc(in);

		if (ch == EOF) {
			bit_write(out, (const char *)(&(comp->cur_node)), comp->nbits, 0);
			comp->cur_node = EOF_CODE;
			bit_write(out, (const char*)(&(comp->cur_node)), comp->nbits, 0);
			bit_flush(out);
			bit_close(out);
			break;
		}

		if (comp->cur_node == ROOT_CODE) {
			comp->cur_node = ch;
			continue;
		}

		update_and_code (ch, comp, out);

		/* Quando viene creato il nuovo dizionario, il primo carattere verrà
		 * messo nella codifica successiva emessa da comp, per cui anche il
		 * decompressore parte dallo stsso carattere
		 * */
		if (comp->hash_size >= ( (DICT_SIZE >> 2) * 3 ) ) {
			if (new_comp == NULL) {
				new_comp = comp_init();
				printf ("New dictionary started\n");
			}

			if (new_comp->cur_node == ROOT_CODE) {
				new_comp->cur_node = ch;
			}
			else {
				update_and_code(ch, new_comp, NULL);
			}
		}

		if (comp->hash_size == DICT_SIZE) {
			bit_write(out, (const char *)(&(comp->cur_node)), comp->nbits, 0);
//			comp->cur_node = EOD_CODE;
//			bit_write(out, (const char*)(&(comp->cur_node)), comp->nbits, 0);
//si manda la codifica zompa dalla quale il decomp può prendere il primo carattere
//per completare l'ultima entry
bit_write(out, (const char *)(&(new_comp->cur_node)), new_comp->nbits, 0);

			printf ("Ultima codifica scritta (prima del purge): %u\n", ultima);
			printf ("Codifica purged: %u\n", comp->cur_node);

			new_comp->cur_node = ROOT_CODE;

			lz78_destroy(comp);
			comp = new_comp;
			new_comp = NULL;

			printf ("Dimensione del nuovo diz.: %u\n\n", comp->hash_size);
flag = 1;
		}

	}

	lz78_destroy(comp);
	lz78_destroy(new_comp);
}

void lz78_compress1(struct lz78_c* comp, FILE* in, struct bitfile* out)
{
	int ch;
	unsigned int index;
unsigned int last_used = 0;
	int ret = 0;
	struct lz78_c *new_comp = NULL;

	for (; ; ) {
		ch = fgetc(in);

		if (ch == EOF){
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
			comp->dict[index].character = (unsigned char)ch;
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
		}

		else if ( (comp->dict[index].character == (unsigned char)ch) &&
				(comp->dict[index].parent_code == comp->cur_node) ) {
			//a match
			comp->cur_node = comp->dict[index].code;
		}
		else {
			user_err("lz78_compress: error in hash function");
		}

		//75% of DICT_SIZE

		if (comp->hash_size >= ( (DICT_SIZE >> 2) * 3) ){
			//creare un nuovo dizionario
			/*continuare con il funzionamento di prima ma aggiungere le stesse
			  info ad entrambi i dizionari*/
			if (new_comp == NULL){
				printf ("Starting new dict\n");
				new_comp = comp_init();
				new_comp->cur_node = SND_CODE;
				bit_write(out, (const char *)(&(new_comp->cur_node)),
									comp->nbits, 0);
				new_comp->cur_node = ROOT_CODE;
			}

			//always verified the first time this new_dict is used
			if (new_comp->cur_node == ROOT_CODE) {
				new_comp->cur_node = ch;
			}

			else {
				index = find_child_node(new_comp->cur_node, ch, new_comp);

				if (new_comp->dict[index].code == EMPTY_NODE_CODE){
					new_comp->dict[index].character = (unsigned char)ch;
					new_comp->dict[index].parent_code = new_comp->cur_node;
					new_comp->dict[index].code = new_comp->d_next;
last_used = index;
					new_comp->d_next++;
					new_comp->hash_size++;
					new_comp->nbits = ceil_log2(new_comp->hash_size);
					new_comp->cur_node = ch;
				}
				else if ((new_comp->dict[index].character == (unsigned char)ch) &&
						(new_comp->dict[index].parent_code == new_comp->cur_node)){
					new_comp->cur_node = new_comp->dict[index].code;
				}
				else{
					user_err("lz78_compress: error in hash function in new dict");
				}
			}
		}

		if (comp->hash_size == DICT_SIZE){
		//if (comp->hash_size == ((1 << BITS) - 1) ) {
			//scrivere sul file compresso la codifica attuale
			//scrivere EOD sul file compresso
			//bzero(hash_table)
			//inizializzare i campi comp->... (stato del compressore)
//			printf ("New Dict\n");
//			bit_write(out, (const char*)(&(comp->cur_node)), comp->nbits, 0);
//			comp->cur_node = EOD_CODE;
//			bit_write(out, (const char*)(&(comp->cur_node)), comp->nbits, 0);
//			bzero(comp->dict, DICT_SIZE * sizeof(struct node) );
//			comp->hash_size = FIRST_CODE - 1;
//			comp->cur_node = ROOT_CODE;
//			comp->d_next = FIRST_CODE;
//			//TODO: formalmente hash_size, però non cambia
//			comp->nbits = ceil_log2(comp->d_next);

//printf ("Ultima codifica scritta prima del cambio: %u\n", copia);
			bit_write(out, (const char*)(&(comp->cur_node)), comp->nbits, 0);

printf ("Ultima entry creata da new_comp:\n");
printf("Parent Code: %u\tCode: %u\tChar: %c\n\n", new_comp->dict[last_used].parent_code, new_comp->dict[last_used].code, new_comp->dict[last_used].character);


			//try
//			new_comp->cur_node = ROOT_CODE;
//			new_comp->cur_node = comp->cur_node;


			comp->cur_node = EOD_CODE;
			bit_write(out, (const char*)(&(comp->cur_node)), comp->nbits, 0);


			printf("New dictionary starts from "
					"hash_size: %u\n", new_comp->hash_size);

ch = fgetc(in);
new_comp->cur_node = ch;
//controlli su char letto
if (ch == EOF_CODE) return;
bit_write(out, (const char*)(&(new_comp->cur_node)), new_comp->nbits, 0);
printf ("Codifica dopo EOD: %u\n", ch);

			lz78_destroy(comp);
			comp = new_comp;
			new_comp = NULL;
		}
	}
	lz78_destroy(comp);
	lz78_destroy(new_comp);
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

void lz78_decompress(struct lz78_c* decomp, FILE* out, struct bitfile* in)
{
	unsigned int code = 0;
	struct d_stack *sequence = stack_init(MAX_SEQUENCE_LENGTH);
	unsigned char leaf_char;
	struct lz78_c *new_d = NULL;
	struct lz78_c *inner_comp = NULL;
	int save = 0;
	int flag = 0;
unsigned int debug = 0;

	for (;;) {
		//sequence->top = -1;
		code = read_next_code(in, decomp->nbits);

		//it happens only if the decomp is empty
		if (decomp->cur_node == ROOT_CODE) {
			putc ((unsigned char)code, out);
			decomp->cur_node = code;
			continue;
		}

		if (code == EOF_CODE) {
			break;
		}

		if (code == SND_CODE) {
			printf ("Starting new dictionary\n");
			flag = 1;
			continue;
		}

		if (code == EOD_CODE) {
			printf ("Codifica letta prima di EOD: %u\n", decomp->cur_node);

printf ("Ultima enrty creata da new_decomp:\n");
printf ("Parent Code: %u\tCode: %u\tChar: %c\n", new_d->dict[new_d->d_next - 1].parent_code, new_d->dict[new_d->d_next - 1].code, new_d->dict[new_d->d_next - 1].character);

code = read_next_code(in, new_d->nbits);
if (code == EOF_CODE) break;
if (code > 255) user_err ("No");
printf ("Codifica dopo EOD: %u\n\n", code);
new_d->cur_node = code;
			//marker nel file decompresso, la codifica prima dovrebbe essere quella stampata
			//putc (0, out);
//			if (inner_comp->cur_node > 255) {
//				printf ("Hash Size: %u\n", new_d->hash_size);
//				code = read_next_code(in, new_d->nbits);
//				decode_stack(sequence, new_d, code);
//				save = sequence->top;
//				//TODO: while qui?
//				//TODO: trovare file come BBT ma + piccolo!
//				code = string_to_code(sequence, inner_comp);
//				if (code == ROOT_CODE) user_err ("Something wrong");
//				new_d->dict[new_d->d_next].code = new_d->d_next;
//				new_d->dict[new_d->d_next].parent_code = new_d->cur_node;
//				new_d->dict[new_d->d_next].character = root_char(code, new_d);
			//try
			//new_d->cur_node = decomp->cur_node;
//				new_d->cur_node = code;
//				new_d->d_next++;
//				new_d->hash_size++;
//				new_d->nbits = ceil_log2(new_d->d_next);
//				//si deve partire dal carattere a cui era rimasto cur_node di inner (??)
//				for (i = save - 1; i >= sequence->top; i--) {
//					//printf ("Vorrei inserire %c\n", sequence->stack[i]);
//					putc (sequence->stack[i], out);
//				}
//				sequence->top = save;
//			}
//			//flush_stack_to_file(sequence, out);
//			if (inner_comp->cur_node > 255)
//				user_err ("Now it should be reset");
			lz78_destroy(decomp);
//			new_d->cur_node = ROOT_CODE;
			decomp = new_d;
			new_d = NULL;
			lz78_destroy(inner_comp);
			inner_comp = NULL;
			flag = 0;
debug = 1;
			continue;
		}

		decomp->dict[decomp->d_next].code = decomp->d_next;
		decomp->dict[decomp->d_next].parent_code = decomp->cur_node;
ultima_d = decomp->cur_node;
		decode_stack (sequence, decomp, code);
		decomp->dict[decomp->d_next].character = stack_top(sequence);
//if (debug == 1) {
//	debug = 0;
//	printf ("\nPrima entry creata dopo cambio:\n");
//	printf ("Parent Code: %u\tCode: %u\tChar: %c\n\n", decomp->dict[decomp->d_next].parent_code, decomp->dict[decomp->d_next].code, decomp->dict[decomp->d_next].character);
//}
		leaf_char = (code < 256) ?
				(unsigned char)code : decomp->dict[code].character;
		stack_bottom (sequence, leaf_char);
		decomp->cur_node = code;
		decomp->d_next++;
		decomp->hash_size++;
		decomp->nbits = ceil_log2(decomp->d_next);

		//TODO: usare questo o leggere codice "inzia un nuovo dizionario"
		if (decomp->hash_size >= ((DICT_SIZE >> 2) * 3 ) ) {
		//if (flag == 1) {
			if (new_d == NULL) {
				new_d = decomp_init();
			}
			if (inner_comp == NULL) {
				inner_comp = comp_init();
			}

			save = sequence->top;
			//gets a new code as if it was given by a new compressor
			while ( (code = string_to_code(sequence, inner_comp)) != ROOT_CODE )
			{
				if (new_d->cur_node == ROOT_CODE) {
					new_d->cur_node = code;
					continue;
				}
				new_d->dict[new_d->d_next].code = new_d->d_next;
				new_d->dict[new_d->d_next].parent_code = new_d->cur_node;
				new_d->dict[new_d->d_next].character = root_char(code, new_d);
				new_d->cur_node = code;
				new_d->d_next++;
				new_d->hash_size++;
				new_d->nbits = ceil_log2(new_d->d_next);
			}
			//the previous loop empties the stack, we restore it
			sequence->top = save;
		}

		if (decomp->hash_size == DICT_SIZE) {
			//stampare dimensione del nuovo dizionario qui e nel compressore
printf ("Ultima codifica scritta: %u\n", ultima_d);
printf ("Ultima codifica letta (la purged): %u\n", decomp->cur_node);
			lz78_destroy(decomp);
			decomp = new_d;
			new_d = NULL;
			lz78_destroy(inner_comp);
			inner_comp = NULL;

//spurge del vecchio dizionario
flush_stack_to_file(sequence, out);
//decomp->cur_node = ROOT_CODE;

//codifica zompa del nuovo: attenzione, la codifica potrebbe non essere zompa
//(potrebbe essere completa e quindi il decomp poteva averla già scritta)
if (decomp->cur_node > 255) {
	//significa che è rimasto in uno stato zompo
	code = read_next_code(in, decomp->nbits);
	decomp->dict[decomp->d_next].code = decomp->d_next;
	decomp->dict[decomp->d_next].parent_code = decomp->cur_node;
	decomp->dict[decomp->d_next].character = root_char(code, decomp);
	decomp->d_next++;
	decomp->hash_size++;
	decomp->nbits = ceil_log2(decomp->d_next);
}

code = read_next_code(in, decomp->nbits);
printf ("Nuova codifica dopo cambio: %u\n", code);
decode_stack(sequence, decomp, code);
decomp->cur_node = code;
flush_stack_to_file(sequence, out);
			printf ("Dimensione del nuovo diz: %u\n\n", decomp->hash_size);
continue;
		}

		flush_stack_to_file(sequence, out);
//		if (new_d == NULL || decomp->hash_size < DICT_SIZE) {
//			flush_stack_to_file(sequence, out);
//		}
	}
	stack_destroy(sequence);
	lz78_destroy(decomp);
	lz78_destroy(new_d);
	lz78_destroy(inner_comp);
}

/**
 * In the decompress function, the hash table is accessed directly using
 * the node code value.
 */
void lz78_decompress1(struct lz78_c* decomp, FILE* out, struct bitfile* in)
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
	struct seq_elem *first = NULL;
	struct seq_elem *last = NULL;
	char last_c = 0;
	struct lz78_c *new_decomp = NULL;
	struct lz78_c *inner_comp = NULL;
	struct codes_queue *cq = NULL;
	struct codes_queue *first_q = NULL;
int flag = 0;


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

//if (flag == 1) {
//	flag = 0;
//	printf ("Dopo cambio: %u\n", read_code);
//}

		if (read_code == EOD_CODE) {
			printf ("lz78_decompress: changing dictionary\n");
printf ("Prima del cambio: %u\n", decomp->cur_node);
flag = 1;
//cq non è stato svuotato propriamente
if (cq != NULL && cq->code != ROOT_CODE) user_err("Problema");

			free (decomp->dict);
			free (decomp);

			decomp = new_decomp;
			new_decomp = NULL;

			printf ("Starting from size: %u\n", decomp->hash_size);

			free (inner_comp->dict);
			free (inner_comp);
			inner_comp = NULL;

			while (sequence->next != NULL) {
				sequence = sequence->next;
				free (sequence->prec);
			}
			free (sequence);

			//TODO: codice ridondante, uguale a prima del ciclo
			//reading first code as initialization, it provides a parent code
//			ret = bit_read(in, (char *)(&read_code), decomp->nbits, 0);
//			while (ret < decomp->nbits) {
//				printf ("lz78_decompress: caution, into the while!\n");
//				ret += bit_read(in, (char *)(&read_code), (decomp->nbits - ret), ret);
//			}
//			decomp->cur_node = read_code;
//			if (read_code == EOF_CODE) {
//				bit_close(in);
//				return;
//			}
//			putc(read_code, out);
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

		first = sequence;
		while (sequence->prec != NULL) {
			putc(sequence->c, out);
			sequence = sequence->prec;
		}
		sequence->c = last_c;
		putc(last_c, out);
		//free(sequence);
		//the leaf node (first in list, last in tree)
		last = sequence;
		//the sub-root node (last in list, first in tree)
		sequence = first;

		//((DICT_SIZE >> 2) * 3) = 75% of DICT_SIZE
		if (decomp->hash_size >= ((DICT_SIZE >> 2) * 3)){
		//if (decomp->d_next >= ((DICT_SIZE >> 2) * 3)){
			/*
			 * Si esegue un compressore con la stringa trovata dalla sequenza
			 * considerata.
			 * Il compressore restituisce la codifica che dovrà essere usata
			 * nel nuovo dizionario
			 * Il compressore ritorna le codifiche che scriverebbe sul
			 * file compresso.
			 * Quindi è come se avessi il file compresso col nuovo dizionario
			 * Da questo costruisco il nuovo dizionario.
			 *
			 * La prima entry del nuovo dizionario nel compressore sarà:
			 * padre < 256, code = FIRST_CODE
			 * es.: <padre: 65, char: 66, code: 258>
			 * La prima codifica letta da file compresso sarà dipendente dal
			 * vecchio dizionario
			 * */
			if (new_decomp == NULL){
				printf("75%% achieved\n");
				new_decomp = decomp_init();
				inner_comp = comp_init();
			}
			//la prima codifica deve essere letta senza scrivere sul dizionario
			if (new_decomp->cur_node == ROOT_CODE) {
				//here only if new_decomp is empty
				//TODO: last si può eliminare se qui e due volte sotto si usa first
				//		(si lascia sequnce andare fino in fondo)
				//for (;sequence!=NULL;sequence=sequence->prec) printf ("SEQ: %u\n", sequence->c);
				cq = string_to_code1(inner_comp, sequence, cq);
				if (cq != NULL) {
					new_decomp->cur_node = cq->code;
printf ("Codifica: %u\n", cq->code);
					first_q = cq;
					cq = cq->next;
					free (first_q);
				}
				else
					user_err("Empty codes_queue, but should be not empty");
			}
			else {
//printf ("Qui\n");
				cq = string_to_code1(inner_comp, sequence, cq);
				first_q = cq;
				while (cq != NULL && cq->code != ROOT_CODE) {
					//for each code found:
					new_decomp->dict[new_decomp->d_next].parent_code =
														new_decomp->cur_node;
					new_decomp->dict[new_decomp->d_next].code =
														new_decomp->d_next;
					new_decomp->dict[new_decomp->d_next].character = cq->c;

//if (flag == 1 && new_decomp->d_next < 10000) printf ("Codifica: %u\n", cq->code);
//if (flag == 1 && new_decomp->d_next > 10000) pause();

					new_decomp->cur_node = cq->code;
					new_decomp->d_next++;
					new_decomp->hash_size++;
					new_decomp->nbits = ceil_log2(new_decomp->d_next);
					//new_decomp->nbits = ceil_log2(new_decomp->hash_size);
					cq->code = ROOT_CODE;
					cq = cq->next;
				}
				cq = first_q;
			}
		}
		sequence = last;
	}
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
	sequence->c = (unsigned char)code;
	return sequence;
}

//ritorna una codifica alla volta su un intero: invece delle bit_write usare
//i return
//Problema: potrebbe ritornare più di una codifica
struct codes_queue *string_to_code1(struct lz78_c* comp,
		struct seq_elem *s,
		struct codes_queue *cq)
{
	int ch;
	unsigned int index;
	//int ret = 0;
	int counter = 0;
	struct seq_elem *seq = s;
	struct codes_queue *q = cq;

	//for (; seq != NULL; seq = seq->prec) {
	while (seq != NULL) {

		ch = (int)seq->c;
//printf ("Carattere: %u\n", seq->c);

		seq = seq->prec;
		counter++;

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
			comp->dict[index].character = (unsigned char)ch;
			comp->dict[index].code = comp->d_next;
			comp->dict[index].parent_code = comp->cur_node;

			q = insert_queue(comp->cur_node, (unsigned char)ch, q);

			comp->cur_node = ch;
			comp->hash_size++;
			comp->d_next++;
			comp->nbits = ceil_log2(comp->hash_size);
		}

		else if ( (comp->dict[index].character == (unsigned char)ch) &&
				(comp->dict[index].parent_code == comp->cur_node) ) {
			//a match
			comp->cur_node = comp->dict[index].code;
			//continue;
		}
		else {
			user_err("lz78_compress: error in hash function");
		}
	}
	return q;
}

struct codes_queue *insert_queue (unsigned int code, unsigned char c, struct codes_queue *cq)
{
	struct codes_queue *q = cq;
	struct codes_queue *begin = cq;

	if (q == NULL) {
		q = (struct codes_queue *)malloc(sizeof(struct codes_queue));
		q->code = code;
		q->c = c;
		q->next = NULL;
		//return 1;
		return q;
	}
	while (q->code != ROOT_CODE && q->next != NULL) {
		q = q->next;
	}
	if (q->code == ROOT_CODE) {
		q->code = code;
		q->c = c;
		//return 2;
		return begin;
	}
	if (q->next == NULL) {
		q->next = (struct codes_queue *)malloc(sizeof(struct codes_queue));
		q = q->next;
		q->next = NULL;
		q->code = code;
		q->c = c;
		//return 3;
		return begin;
	}
user_err("QUI");
	return 0;
}

unsigned int read_next_code(struct bitfile *in, unsigned int n_bits)
{
	unsigned int code = 0;
	int ret = 0;

	ret = bit_read(in, (char *)(&code), n_bits, 0);
	while (ret < n_bits) {
		ret += bit_read(in, (char *)(&code), (n_bits - ret), ret);
	}
	return code;
}

void decode_stack (struct d_stack *s, struct lz78_c *d, unsigned int code)
{
	if (!stack_is_empty(s))
			user_err ("decode_stack: the stack should be empty");
	if (code > d->d_next) {
		printf ("\nRequested code: %u\n", code);
		printf ("Dict size: %u\n", d->hash_size);
		printf ("Dnext: %u\n", d->d_next);
		user_err("Code not found in dictionary");
	}
	while (code >= FIRST_CODE) {
		stack_push(s, d->dict[code].character);
		code = d->dict[code].parent_code;
	}
	stack_push(s, (unsigned char)code);
}

void lz78_destroy (struct lz78_c *dd)
{
	if (dd != NULL) {
		if (dd->dict != NULL)
			free (dd->dict);
		free (dd);
	}
}

/**
 * Data una stringa, *estrae* un carattere alla volta e se non trova match
 * emette la codifica come se scrivesse un file compresso.
 * Se invece la stringa finisce il compressore rimane al nodo corrente
 * dell'albero (lo stato rimane salvato in attesa della successiva stringa)
 */
unsigned int string_to_code (struct d_stack *s, struct lz78_c *comp)
{
	int ch = 0;
	unsigned int index = 0;
	unsigned int code = ROOT_CODE;

	while (!stack_is_empty(s)) {
		ch = stack_pop(s);
		code = ROOT_CODE;

		if (comp->cur_node == ROOT_CODE) {
			comp->cur_node = ch;
			continue;
		}
		index = find_child_node(comp->cur_node, ch, comp);

		if (index > DICT_SIZE) {
			user_err ("find_child_node: index out of bound");
		}

		if (comp->dict[index].code == EMPTY_NODE_CODE){
			//the child doesn't exist
			comp->dict[index].character = (unsigned char)ch;
			comp->dict[index].code = comp->d_next;
			comp->dict[index].parent_code = comp->cur_node;

			code = comp->cur_node;

			comp->cur_node = ch;
			//TODO: basta aggiornare solo cur_node e d_next
			comp->hash_size++;
			comp->d_next++;
			comp->nbits = ceil_log2(comp->hash_size);
			break;
		}

		else if ( (comp->dict[index].character == (unsigned char)ch) &&
				(comp->dict[index].parent_code == comp->cur_node) ) {
			//a match
			comp->cur_node = comp->dict[index].code;
			//continue;
		}
		else {
			user_err("lz78_compress: error in hash function");
		}
	}
	return code;
}

unsigned char root_char (unsigned int code, struct lz78_c *c)
{
	while (code > 255) {
		code = c->dict[code].parent_code;
	}
	return code;
}

void update_and_code (int ch,struct lz78_c *comp, struct bitfile *out)
{
	unsigned int index = 0;
	int ret = 0;

	index = find_child_node(comp->cur_node, (unsigned int)ch, comp);

	if (index > DICT_SIZE) {
		user_err ("find_child_node: index out of bound");
	}

	if (comp->dict[index].code == EMPTY_NODE_CODE){
		//the child doesn't exist
		comp->dict[index].character = (unsigned char)ch;
		comp->dict[index].code = comp->d_next;
		comp->dict[index].parent_code = comp->cur_node;

		if (out != NULL) {
			ultima = comp->cur_node;
			if (flag == 1) {printf ("Nuova dopo cambio: %u\n", ultima); flag = 0;}
			ret = bit_write(out, (const char *)(&(comp->cur_node)),
					comp->nbits, 0);
			if (ret != comp->nbits)
				user_err ("lz78_compress: not all bits written");
		}

		//next node will be the last not matching
		comp->cur_node = ch;
		comp->hash_size++;
		comp->d_next++;
		comp->nbits = ceil_log2(comp->hash_size);
	}

	else if ( (comp->dict[index].character == (unsigned char)ch) &&
			(comp->dict[index].parent_code == comp->cur_node) ) {
		//a match
		comp->cur_node = comp->dict[index].code;
	}
	else {
		user_err("lz78_compress: error in hash function");
	}
}
