#include "lz78.h"


struct lz78_c* lz78c_init()
{
	uint32_t size = DICT_SIZE;
	struct node* ht = NULL;
	struct lz78_c* comp;

	ht = (struct node*)malloc(size * sizeof(struct node));
	if (ht == NULL){
		sys_err("comp_init: error allocating hash table");
	}
	bzero(ht, size * sizeof(struct node));

	comp = (struct lz78_c*)malloc(sizeof(struct lz78_c));
	if (comp == NULL){
		sys_err("comp_init: error allocating lz78_c struct");
	}
	bzero(comp, sizeof(struct lz78_c));

	comp->cur_node = ROOT_CODE;
	comp->d_next = FIRST_CODE;
	comp->dict = ht;
	//Effective hash table size is 0, but we consider the presence of characters
	//from 0 to 255, which is implicit
	comp->hash_size = FIRST_CODE - 1;
	comp->nbits = ceil_log2(comp->d_next);

	return comp;
}

//from "The Data Compression Book"
uint32_t find_child_node (uint32_t parent_code,
		unsigned int child_char,
		struct lz78_c* comp)
{
	uint32_t index;
	int32_t offset;

	//shifting this way, child char is extended at BITS bit
	index = (child_char << ( BITS - 8 )) ^ parent_code;
	if (index == 0) {
		offset = 1;
	}
	else {
		offset = DICT_SIZE - index;
	}
	for ( ; ; ) {

		//empty node
		if (comp->dict[index].code == EMPTY_NODE_CODE)
			return((uint32_t)index);

		//match
		if (comp->dict[index].parent_code == parent_code &&
				comp->dict[index].character == (u_char)child_char)
			return(index);

		if ((int32_t) index >= offset)
			index -= offset;
		else
			index += DICT_SIZE - offset;
	}
}

void lz78_compress(struct lz78_c* comp, FILE* in, struct bitfile* out, int aexp)
{
	int ch = 0;
	struct lz78_c *new_comp = NULL;
	long int source_length = 0;
	unsigned int w_bits = 0;

	source_length = file_length(fileno(in));

	//inform decomp of BITS used
	bit_write(out, (const char *)(&(BITS)), 8, 0);

	for (;;) {
		ch = fgetc(in);

		if (ch == EOF) {
			//output the current code, then EOF_CODE
			bit_write(out, (const char *)(&(comp->cur_node)), (uint32_t)comp->nbits, 0);
			comp->cur_node = EOF_CODE;
			bit_write(out, (const char*)(&(comp->cur_node)), (uint32_t)comp->nbits, 0);
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

		//new dictionary management
		if (comp->hash_size >= ( (DICT_SIZE >> 2) * 3 ) ) {
			if (new_comp == NULL) {
				new_comp = lz78c_init();
				if (verbose_mode)
					printf ("Compression: new dictionary started at size"
							" %u\n", comp->hash_size);
			}

			if (new_comp->cur_node == ROOT_CODE) {
				new_comp->cur_node = ch;
			}
			else {
				update_and_code(ch, new_comp, NULL, NULL);
			}
		}

		//dictionary swap
		//Note: DICT_SIZE > (1<<BITS)-1, plus 255 nodes are only implicitly full
		//so there are some nodes still not used, this reduces collisions.
		if (comp->hash_size == ((1 << BITS) - 1 )) {
			//current code of the "old" dictionary
			bit_write(out, (const char *)(&(comp->cur_node)), comp->nbits, 0);

			//the decompressor is awaiting for this code to fill its last entry
			//before the swap
			bit_write(out, (const char *)(&(new_comp->cur_node)),
					(uint32_t)new_comp->nbits, 0);

			new_comp->cur_node = ROOT_CODE;
			lz78_destroy(comp);
			comp = new_comp;
			new_comp = NULL;

			//another char is read here because if it is EOF, we don't want
			//to output cur_node as it would happen if we proceed from the
			//beginning of the loop
			ch = fgetc(in);
			if (ch == EOF) {
				comp->cur_node = EOF_CODE;
				bit_write(out, (const char*)(&(comp->cur_node)), (uint32_t)comp->nbits, 0);
				bit_flush(out);
				bit_close(out);
				break;
			}
			else {
				comp->cur_node = ch;
			}
			if (verbose_mode)
				printf ("Compression: dictionary swapped, starting from"
						" %u entries\n", comp->hash_size);
		}
	}

	lz78_destroy(comp);
	lz78_destroy(new_comp);
}

uint8_t ceil_log2(uint32_t x)
{
	uint8_t cont = 0;

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
	uint32_t i = 0;

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

void lz78_decompress(struct lz78_c* decomp, FILE* out, struct bitfile* in)
{

	uint32_t code = 0;
	struct d_stack *sequence = stack_init(MAX_SEQUENCE_LENGTH);
	u_char leaf_char;
	struct lz78_c *new_d = NULL;
	struct lz78_c *inner_comp = NULL;

	for (;;) {
		code = read_next_code(in, decomp->nbits);

		if (code == EOF_CODE) {
			break;
		}

		//it happens only if the decomp is empty
		if (decomp->cur_node == ROOT_CODE) {
			putc ((u_char)code, out);
			decomp->cur_node = code;
			continue;
		}

		decomp->dict[decomp->d_next].code = decomp->d_next;
		decomp->dict[decomp->d_next].parent_code = decomp->cur_node;
		decode_stack (sequence, decomp, code);
		decomp->dict[decomp->d_next].character = stack_top(sequence);

		//last char of the sequence is wrong if we decoded the code just written
		leaf_char = (code < 256) ?
				(u_char)code : decomp->dict[code].character;
		stack_bottom (sequence, leaf_char);
		decomp->cur_node = code;
		decomp->d_next++;
		decomp->hash_size++;
		decomp->nbits = ceil_log2(decomp->d_next);

		if (decomp->hash_size >= ((DICT_SIZE >> 2) * 3 ) ) {

			if (new_d == NULL) {
				new_d = lz78c_init();
				if (verbose_mode)
					printf ("Decompression: new dictionary started\n");
			}
			if (inner_comp == NULL) {
				inner_comp = lz78c_init();
			}

			manage_new_dictionary(new_d, inner_comp, sequence);
		}

		if (decomp->hash_size == ((1 << BITS) - 1) ) {
			lz78_destroy(decomp);
			decomp = new_d;
			new_d = NULL;
			lz78_destroy(inner_comp);
			inner_comp = NULL;

			//last code purged from old compressor
			flush_stack_to_file(sequence, out);

			//the code to complete the last entry before the switch
			//(not that "new_d" is now "decomp")
			code = read_next_code(in, decomp->nbits);
			decomp->dict[decomp->d_next].code = decomp->d_next;
			decomp->dict[decomp->d_next].parent_code = decomp->cur_node;
			decomp->dict[decomp->d_next].character = root_char(code, decomp);
			decomp->d_next++;
			decomp->hash_size++;
			decomp->nbits = ceil_log2(decomp->d_next);

			//this is the first code output by the new compressor
			code = read_next_code(in, ceil_log2(decomp->hash_size));

			if (code == EOF_CODE) {
				break;
			}

			if (code < 256) {
				//next entry must be child of this code
				decomp->cur_node = code;
				putc((u_char)code, out);
			}

			else {
				decode_stack(sequence, decomp, code);

				//if the new dictionary just swapped is already bigger than 75%
				//a new one must be created!
				if (decomp->hash_size >= ((DICT_SIZE >> 2) * 3 ) ) {
					if (new_d == NULL) {
						new_d = lz78c_init();
					}
					if (inner_comp == NULL) {
						inner_comp = lz78c_init();
					}

					//at compressor side the first char received by the new dict
					//is the second one read after the switch, so we move top
					sequence->top--;
					manage_new_dictionary(new_d, inner_comp, sequence);
					sequence->top++;
				}

				//next entry must be child of this code
				decomp->cur_node = code;
				flush_stack_to_file(sequence, out);
			}
			if (verbose_mode)
				printf ("Decompression: dictionary swapped, starting from "
						"%u entries\n", decomp->hash_size);
			continue;
		}
		flush_stack_to_file(sequence, out);
	}
	stack_destroy(sequence);
	lz78_destroy(decomp);
	lz78_destroy(new_d);
	lz78_destroy(inner_comp);
}

uint32_t read_next_code(struct bitfile *in, uint8_t n_bits)
{
	uint32_t code = 0;
	int ret = 0;

	ret = bit_read(in, (char *)(&code), (uint32_t)n_bits, 0);
	while (ret < n_bits) {
		ret += bit_read(in, (char *)(&code), (uint32_t)(n_bits - ret), ret);
	}
	return code;
}

void decode_stack (struct d_stack *s, struct lz78_c *d, uint32_t code)
{
	if (!stack_is_empty(s))
			user_err ("decode_stack: the stack should be empty");
	if (code > d->d_next) {
		printf ("\nRequested code: %u\n", code);
		printf ("Dict size: %u\n", d->hash_size);
		printf ("Dnext: %u\n", d->d_next);
		printf ("Nbits: %u\n", d->nbits);
		user_err("Code not found in dictionary");
	}
	while (code >= FIRST_CODE) {
		stack_push(s, d->dict[code].character);
		code = d->dict[code].parent_code;
	}
	stack_push(s, (u_char)code);
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
 * Reads one char from stack, if a match is found, reads next char, else
 * outputs the code of the last matching node and reads next char
 */
uint32_t string_to_code (struct d_stack *s, struct lz78_c *comp)
{
	int32_t ch = 0;
	uint32_t index = 0;
	uint32_t code = ROOT_CODE;

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
			comp->dict[index].character = (u_char)ch;
			comp->dict[index].code = comp->d_next;
			comp->dict[index].parent_code = comp->cur_node;

			code = comp->cur_node;

			comp->cur_node = ch;
			comp->hash_size++;
			comp->d_next++;
			comp->nbits = ceil_log2(comp->hash_size);
			break;
		}

		else if ( (comp->dict[index].character == (u_char)ch) &&
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

u_char root_char (uint32_t code, struct lz78_c *c)
{
	while (code > 255) {
		code = c->dict[code].parent_code;
	}
	return (u_char)code;
}

void update_and_code (int ch,struct lz78_c *comp, struct bitfile *out,
		unsigned int *wb)
{
	uint32_t index = 0;
	int ret = 0;

	index = find_child_node(comp->cur_node, (unsigned int)ch, comp);

	if (index > DICT_SIZE) {
		printf ("Index: %u\n", index);
		user_err ("find_child_node: index out of bound");
	}

	if (comp->dict[index].code == EMPTY_NODE_CODE){
		//the child doesn't exist
		comp->dict[index].character = (u_char)ch;
		comp->dict[index].code = comp->d_next;
		comp->dict[index].parent_code = comp->cur_node;

		if (out != NULL) {
			ret = bit_write(out, (const char *)(&(comp->cur_node)),
					(uint32_t)comp->nbits, 0);
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

	else if ( (comp->dict[index].character == (u_char)ch) &&
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

void manage_new_dictionary (struct lz78_c *new_d, struct lz78_c *inner_comp,
		struct d_stack *sequence)
{
	int32_t save = 0;
	uint32_t code = 0;

	save = sequence->top;
	//gets a new code as if it was given by a compressor
	while ( (code = string_to_code(sequence, inner_comp)) != ROOT_CODE )
	{
		if ((new_d)->cur_node == ROOT_CODE) {
			(new_d)->cur_node = code;
			continue;
		}
		(new_d)->dict[(new_d)->d_next].code = (new_d)->d_next;
		(new_d)->dict[(new_d)->d_next].parent_code = (new_d)->cur_node;
		(new_d)->dict[(new_d)->d_next].character = root_char(code, (new_d));
		(new_d)->cur_node = code;
		(new_d)->d_next++;
		(new_d)->hash_size++;
		(new_d)->nbits = ceil_log2((new_d)->d_next);
	}
	//the previous loop empties the stack, we restore it
	sequence->top = save;
}

void set_size (uint8_t bits)
{
	BITS = 0;
	DICT_SIZE = 0;

	if (bits < MIN_BITS){
		BITS = MIN_BITS;
	}
	else if (bits > MAX_BITS){
		BITS = MAX_BITS;
	}
	else{
		BITS = bits;
	}
	DICT_SIZE = dict_sizes[BITS - MIN_BITS];

	return;
}
