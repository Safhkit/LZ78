#include <stdlib.h>
#include "utility.h"

#ifndef __stack__h_
#define __stack__h_

struct d_stack {
	u_char* stack;
	uint32_t dim;
	int32_t top;
};

struct d_stack *stack_init(uint32_t dim);
void stack_push(struct d_stack *s, u_char c);
u_char stack_pop(struct d_stack *s);
void stack_destroy(struct d_stack *s);
uint8_t stack_is_empty(struct d_stack *s);
uint8_t stack_is_full(struct d_stack *s);
u_char stack_top(struct d_stack *s);
void stack_bottom(struct d_stack *s, u_char c);
void flush_stack_to_file (struct d_stack *s, FILE *out);

#endif
