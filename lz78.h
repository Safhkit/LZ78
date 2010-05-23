#include "bit_io.h"
#include "utility.h"
#include <string.h>
#include "stack.h"

#ifndef __LZ78_H__
#define __LZ78_H__

/**
 * Codes from 0 to 255 are implicitly present in the dictionary.
 * Codes from 255 to FIRST_CODE can be written in compressed file and can
 * carry different meanings.
 * Codes in hash table start from FIRST_CODE, so 0 can be used to mark empty
 * entries.
 */

//it is the maximum length of a code (min. length is 9)
uint8_t BITS;
//it must be a prime number (to reduce collisions), slightly bigger than 2^BITS
uint32_t DICT_SIZE;

#define MAX_SEQUENCE_LENGTH ((DICT_SIZE  ))// >> 8) + 1)

//root node's code, used as cur_node value to notify the beginning of a new
//sequence
#define ROOT_CODE 256

//code for empty nodes, using 0 allows to initialize the hash table with bzero()
#define EMPTY_NODE_CODE 0

//end of file signal
#define EOF_CODE 257

//first available code when the hash table is created
#define FIRST_CODE 258

//determines the frequency at which the expansion test is performed
//(i.e. every 1600 bits written to compressed file)
#define EXPANSION_TEST_BIT_GRANULARITY 1600

//Minimum maximum code length the user can specify
#define MIN_BITS 10

#define MAX_BITS 21

//dict_sizes array dimension
#define NUM_SIZES 12

//possible choices for DICT_SIZE, each one corresponding to a BITS value
//approximately each choice is 2^BITS
#define SIZE_VALUES 1051, 2053, 4133, 8209, 16411, 35023, 65587, 131143, 262193, 524411, 1048681, 2097169

//maps BITS and DICT_SIZE values
static const uint32_t dict_sizes[NUM_SIZES] = {SIZE_VALUES};

int verbose_mode;

struct node {
	//ranges from FIRST_CODE to 2^21
	uint32_t code;
	u_char character;
	//ranges from 0 to 2^21
	uint32_t parent_code;
};

struct lz78_c {
	struct node* dict;
	uint32_t cur_node;

	/**
	 * Next code to use when inserting a new node in the hash table
	 */
	uint32_t d_next;

	/**
	 * Current hash table size
	 */
	uint32_t hash_size;

	/**
	 * Codes length in bits, it depends on hash_size
	 */
	uint8_t nbits;
};

struct lz78_c* lz78c_init();
uint32_t find_child_node(uint32_t parent_code,
		unsigned int child_char,
		struct lz78_c* comp);

void lz78_compress(struct lz78_c* c, FILE* in, struct bitfile* out, int aexp);

void lz78_decompress(struct lz78_c* c, FILE* out, struct bitfile* in);

/**
 * Utility function which calculates the ceiling of a base 2 log of an integer
 */
uint8_t ceil_log2(uint32_t x);

/**
 * Reads n_bits from file, used in decompression
 */
uint32_t read_next_code(struct bitfile *in, uint8_t n_bits);

/**
 * Given a code, builds the decoded string in reverse fashion into a stack
 * Used in decompression
 */
void decode_stack (struct d_stack *s, struct lz78_c *d, uint32_t code);

void lz78_destroy(struct lz78_c *s);

/**
 * Given a string, returns the code a compressor, in a certain state,
 * would output
 * Used in decompression to build a new dictionary which will be swapped
 */
uint32_t string_to_code (struct d_stack *s, struct lz78_c *comp);

/**
 * Given a code returns the root char, which is the first of the decoded string
 */
u_char root_char (uint32_t code, struct lz78_c *c);

/**
 * Updates the dictionary and eventually writes a new code
 * @ param[in/out] wb is to keep the number of written bits in compressed file,
 * to perform the anti expansion check with some granularity different to
 * "every written bit"
 */
void update_and_code (int ch, struct lz78_c *comp, struct bitfile *out,
		unsigned int *wb);

/**
 * Update a new dictionary which will be used when the on in use will be full
 * Used in decompression
 */
void manage_new_dictionary (struct lz78_c *new_d, struct lz78_c *inner_comp,
		struct d_stack *sequence);

/**
 * It only informs that compressed file is bigger than source file
 */
int anti_expand (unsigned int *wb, struct bitfile *out, long int sfl);

/**
 * bits is given by the user as the maximum code length.
 * This is used to control the size of the dictionary which is slightly bigger
 * than 2^bits
 */
void set_size(uint8_t bits);

/**
 * Prints hash table
 */
void print_comp_ht(struct lz78_c* comp);


#endif

