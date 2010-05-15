#include "lz78.h"

//TODO: se utente mette BITS > 32 (ovvero DICT_SIZE > 2^32 - 1), le codifiche
//		non stanno più sugli interi e non funziona + niente. Fare controllo
//		sulla dim max del dizionario che è 2^32 - 1
//TODO: mettere contatore per le collisioni
//TODO: algoritmo per primo numero primo più grande di un numero dato
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
	bzero(ht, size * sizeof(struct node));

	dict = (struct lz78_c*)malloc(sizeof(struct lz78_c));
	if (dict == NULL){
		sys_err("comp_init: error allocating lz78_c struct");
	}
	bzero(dict, sizeof(struct lz78_c));

	dict->dict = ht;
	//Effective hash table size is 0, but we consider the presence of characters
	//from 0 to 255, which is implicit
	dict->hash_size = FIRST_CODE - 1;
	dict->d_next = FIRST_CODE;
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
	bzero(ht, size * sizeof(struct node));

	decomp = (struct lz78_c*)malloc(sizeof(struct lz78_c));
	if (decomp == NULL){
		sys_err("decomp_init: error allocating lz78_c struct");
	}
	bzero(decomp, sizeof(struct lz78_c));

	decomp->cur_node = ROOT_CODE;
	decomp->d_next = FIRST_CODE;
	decomp->dict = ht;
	decomp->hash_size = FIRST_CODE - 1;
	decomp->nbits = ceil_log2(decomp->d_next);

	return decomp;
}

//unsigned int find_child_node(unsigned int parent_code,
//		unsigned int child_char,
//		struct lz78_c* comp)
//{
//	unsigned int index = 0;
//	unsigned int conflicts = 0;
//
//	index = (child_char << (BITS - 8) ) ^ parent_code;
//
//	for (; ;) {
//		if (index > DICT_SIZE) {
//			index = 0;
//		}
//		if (conflicts > DICT_SIZE) {
//			printf ("Hash size: %u\n", comp->hash_size);
//			pause();
//		}
//		if (comp->dict[index].code == EMPTY_NODE_CODE)
//			return index;
//		if (comp->dict[index].parent_code == parent_code &&
//				comp->dict[index].character == (unsigned char) child_char)
//			return index;
//		index++;
//		conflicts++;
//	}
//}

//from "The Data Compression Book"
unsigned int find_child_node (unsigned int parent_code,
		unsigned int child_char,
		struct lz78_c* comp)
{
	unsigned int index;
	int offset;

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
//		//TODO: if di debug
//		if (conflicts > DICT_SIZE) {
//			printf ("Conflitti: %u\n", conflicts);
//			printf ("Hash size: %u\n", comp->hash_size);
//			printf ("Carattere passato: %u\n", child_char);
//			printf ("Parent code: %u\n", parent_code);
//			printf ("Indice: %u\n", index);
//			pause();
//		}
//
//		//TODO: if di debug
//		if (index > DICT_SIZE) {
//			printf ("Index: %u\n", index);
//			pause();
//		}

		if (comp->dict[index].code == EMPTY_NODE_CODE) {
			//empty node
			return((unsigned int)index);
		}
		if (comp->dict[index].parent_code == parent_code &&
				comp->dict[index].character == (unsigned char)child_char) {
			//match
			return(index);
		}
		if ((int) index >= offset) {
			index -= offset;
		}
		else {
			index += DICT_SIZE - offset;
		}
	}
}

void lz78_compress(struct lz78_c* comp, FILE* in, struct bitfile* out, int aexp)
{
	int ch = 0;
	struct lz78_c *new_comp = NULL;
	long int source_length = 0;
	unsigned int w_bits = 0;

	source_length = file_length(fileno(in));
//	printf ("Source file length: %ld\n", source_length);

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

		update_and_code (ch, comp, out, &w_bits);

		//if antiexpand flag (aexp) was set, compression stops when expansion
		//happens
		if (aexp && anti_expand (&w_bits, out, source_length))
			break;

		if (comp->hash_size >= ( (DICT_SIZE >> 2) * 3 ) ) {
			if (new_comp == NULL) {
				new_comp = comp_init();
			}

			if (new_comp->cur_node == ROOT_CODE) {
				new_comp->cur_node = ch;
			}
			else {
				update_and_code(ch, new_comp, NULL, NULL);
			}
		}

//		if (comp->hash_size == DICT_SIZE) {
		if (comp->hash_size == ((1 << BITS) - 1 )) {
			bit_write(out, (const char *)(&(comp->cur_node)), comp->nbits, 0);

			//the decompressor is awaiting for this code to fill its last entry
			//before the swap
			bit_write(out, (const char *)(&(new_comp->cur_node)),
					new_comp->nbits, 0);

			new_comp->cur_node = ROOT_CODE;

			lz78_destroy(comp);
			comp = new_comp;
			new_comp = NULL;
			printf ("New dictionary selected, starting from %u entries\n",
					comp->hash_size);
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

	if (cont < 8) {
		printf ("ceil_log2: returning %d\n", cont);
		user_err ("At least 9 bits should always be used");
	}
	return cont;
}

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

		decomp->dict[decomp->d_next].code = decomp->d_next;
		decomp->dict[decomp->d_next].parent_code = decomp->cur_node;
		decode_stack (sequence, decomp, code);
		decomp->dict[decomp->d_next].character = stack_top(sequence);
		leaf_char = (code < 256) ?
				(unsigned char)code : decomp->dict[code].character;
		stack_bottom (sequence, leaf_char);
		decomp->cur_node = code;
		decomp->d_next++;
		decomp->hash_size++;
		decomp->nbits = ceil_log2(decomp->d_next);

		if (decomp->hash_size >= ((DICT_SIZE >> 2) * 3 ) ) {
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

//		if (decomp->hash_size == DICT_SIZE) {
		if (decomp->hash_size == ((1 << BITS) - 1) ) {
			lz78_destroy(decomp);
			decomp = new_d;
			new_d = NULL;
			lz78_destroy(inner_comp);
			inner_comp = NULL;

			//last code purged from old compressor
			flush_stack_to_file(sequence, out);

//			if (decomp->cur_node > 255) {
				//inner_comp was in an unclear state
				//if inner_comp already added this one?
				code = read_next_code(in, decomp->nbits);
				decomp->dict[decomp->d_next].code = decomp->d_next;
				decomp->dict[decomp->d_next].parent_code = decomp->cur_node;
				decomp->dict[decomp->d_next].character = root_char(code, decomp);
				decomp->d_next++;
				decomp->hash_size++;
				decomp->nbits = ceil_log2(decomp->d_next);
//			}

			code = read_next_code(in, decomp->nbits);
			decode_stack(sequence, decomp, code);
			decomp->cur_node = code;
			flush_stack_to_file(sequence, out);
			printf ("New dictionary selected, starting from %u entries\n",
					decomp->hash_size);
			continue;
		}
		flush_stack_to_file(sequence, out);
	}
	stack_destroy(sequence);
	lz78_destroy(decomp);
	lz78_destroy(new_d);
	lz78_destroy(inner_comp);
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
 * Se invece la stringa finisce, il compressore rimane al nodo corrente
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
			comp->hash_size++;
			comp->d_next++;
			comp->nbits = ceil_log2(comp->hash_size);
			break;
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
	return code;
}

unsigned char root_char (unsigned int code, struct lz78_c *c)
{
	while (code > 255) {
		code = c->dict[code].parent_code;
	}
	return code;
}

void update_and_code (int ch,struct lz78_c *comp, struct bitfile *out, unsigned int *wb)
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
			ret = bit_write(out, (const char *)(&(comp->cur_node)),
					comp->nbits, 0);
			if (ret != comp->nbits)
				user_err ("lz78_compress: not all bits written");
			if (wb != NULL)
				*wb += ret;
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

int anti_expand (unsigned int *wb, struct bitfile *out, long int sfl)
{
	long int dest_file_length;

	if (*wb < EXPANSION_TEST_BIT_GRANULARITY) {
		return 0;
	}

	dest_file_length = file_length(out->fd);
	if (dest_file_length > sfl) {
		//expansion happened
		printf ("Compressed file bigger than the original one\n");
		bit_flush(out);
		bit_close(out);
		return 1;
	}
	else {
		*wb = 0;
		return 0;
	}
}
