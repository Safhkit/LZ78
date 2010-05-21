#include <stdlib.h>
#include "utility.h"

#ifndef __stack__h_
#define __stack__h_

/*
 * Element of a stack used for keep a sequence in the dictionary.
 * Characters are kept from dictionary and put into the stack in a opposite
 * order respect of the right one. So with a LIFO fashion is possible to
 * retrieve the right order.
 *
 * @param stack: character
 * @param dim:	 actual stack size
 * @param top:	 indicate the top of the stack (-1) or the character's position
 *
 */
struct d_stack {
	u_char* stack;
	uint32_t dim;
	int32_t top;
};


/*
 * stack_init:
 *
 * Creates and initilizes a stack
 *
 * @param dim:	 stack size
 *
 * @return:		 a pointer to the stack
 *
 */
struct d_stack *stack_init(uint32_t dim);

/*
 * stack_push:
 *
 * Pushes an element into the stack, if it isn't full.
 *
 * @param s:	 stack pointer
 * @param c:	 character to be insert
 *
 */
void stack_push(struct d_stack *s, u_char c);

/*
 * stack_pop:
 *
 * Pops an element from the stack, if it isn't empty.
 *
 * @param s:	 stack pointer
 *
 * @return:		 character in top position
 *
 */
u_char stack_pop(struct d_stack *s);

/*
 * stack_destroy:
 *
 * Destroys the stack.
 *
 * @param s:	 stack to be destroy
 *
 */
void stack_destroy(struct d_stack *s);

/*
 * stack_is_empty:
 *
 * Checks if the stack is empty.
 *
 * @return:		 (if > 0: true; = 0: false)
 *
 */
uint8_t stack_is_empty(struct d_stack *s);

/*
 * stack_is_full:
 *
 * Checks if the stack is full.
 *
 * @return:		 (if > 0: true; = 0: false)
 *
 */
uint8_t stack_is_full(struct d_stack *s);

/*
 * stack_top:
 *
 * Returns character in top position without pops it.
 *
 * @return:		 character in top position
 *
 */
u_char stack_top(struct d_stack *s);

/*
 * stack_bottom:
 *
 * Inserts an element in the first position of the stack (with index 0).
 *
 * @param s:	 stack pointer
 * @param c:	 character to be insert
 *
 */
void stack_bottom(struct d_stack *s, u_char c);

/*
 * flush_stack_to_file:
 *
 * Writes all the characters in the stack (i.e. a sequence of characters) in
 * the file passed as second argument.
 *
 * @param s:	 stack pointer
 * @param out:	 output file
 *
 */
void flush_stack_to_file (struct d_stack *s, FILE *out);

#endif
