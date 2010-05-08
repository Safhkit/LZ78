#include <stdlib.h>
#include "utility.h"

#ifndef __stack__h_
#define __stack__h_

struct d_stack {
	unsigned char* stack;
	unsigned int dim;
	int top;
};

struct d_stack *stack_init(unsigned int dim);
void stack_push(struct d_stack *s, unsigned char c);
unsigned char stack_pop(struct d_stack *s);
void stack_destroy(struct d_stack *s);
int stack_is_empty(struct d_stack *s);
int stack_is_full(struct d_stack *s);
unsigned char stack_top(struct d_stack *s);
void stack_bottom(struct d_stack *s, unsigned char c);
void flush_stack_to_file (struct d_stack *s, FILE *out);

#endif
