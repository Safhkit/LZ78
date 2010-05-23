#include <stdlib.h>
#include "utility.h"

#ifndef __stack__h_
#define __stack__h_

/*
 * Structure that implements a stack of characters.
 *
 * stack: stack pointer
 * dim:	 stack size
 * top:	 top position of the stack (-1 if the stack is empty)
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
 * Creates and initializes a stack
 *
 * @param dim:	 stack size
 *
 * @return:		 a pointer to the structure describing the stack (d_stack)
 *
 */
struct d_stack *stack_init(uint32_t dim);

/*
 * stack_push:
 *
 * Pushes an element into the stack, if it isn't full.
 *
 * @param s:	 d_stack pointer
 * @param c:	 character to be inserted
 *
 */
void stack_push(struct d_stack *s, u_char c);

/*
 * stack_pop:
 *
 * Pops an element from the stack, if it isn't empty.
 *
 * @param s:	 d_stack pointer
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
 * @param s:	 d_stack to be destroyed
 *
 */
void stack_destroy(struct d_stack *s);

/*
 * stack_is_empty:
 *
 * Checks if the stack is empty.
 *
 * @return 1 if the stack is empty, 0 otherwise.
 *
 */
uint8_t stack_is_empty(struct d_stack *s);

/*
 * stack_is_full:
 *
 * Checks if the stack is full.
 *
 * @return 1 if top is equal to (dim-1).
 *
 */
uint8_t stack_is_full(struct d_stack *s);

/*
 * stack_top:
 *
 * Returns character in top position without popping it.
 *
 * @return:		 character in top position
 *
 */
u_char stack_top(struct d_stack *s);

/*
 * stack_bottom:
 *
 * Inserts an element in the first position of the stack (index 0).
 *
 * @param s:	 d_stack pointer
 * @param c:	 character to be inserted
 *
 */
void stack_bottom(struct d_stack *s, u_char c);

/*
 * flush_stack_to_file:
 *
 * Writes all the characters in the stack (i.e. a sequence of characters) in
 * the file passed as second argument.
 *
 * @param s:	 d_stack pointer
 * @param out:	 output file
 *
 */
void flush_stack_to_file (struct d_stack *s, FILE *out);

#endif
