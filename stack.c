#include "stack.h"


struct d_stack *stack_init(uint32_t dim)
{
	struct d_stack * s = (struct d_stack *)malloc(sizeof (struct d_stack));
	s->stack = (u_char *)malloc(sizeof(u_char) * dim);
	s->top = -1;
	s->dim = dim;
	return s;
}

void stack_push(struct d_stack *s, u_char elem)
{
	if (stack_is_full(s))
		user_err ("Decode stack full!");
	s->top++;
	s->stack[s->top] = elem;
}

u_char stack_pop(struct d_stack *s){
	if (stack_is_empty(s))
		user_err("Cannot pop from an empty stack");
	return s->stack[s->top--];
}

uint8_t stack_is_empty(struct d_stack *s)
{
	return (s->top < 0);
}

uint8_t stack_is_full(struct d_stack *s)
{
	return ((s->top) >= (int32_t)(s->dim - 1));
}

void stack_destroy(struct d_stack *s)
{
	if (s != NULL){
		if (s->stack != NULL)
			free (s->stack);
		free (s);
	}
}

u_char stack_top(struct d_stack *s)
{
	return s->stack[s->top];
}

void stack_bottom(struct d_stack *s, u_char c)
{
	s->stack[0] = c;
}

void flush_stack_to_file (struct d_stack *s, FILE *out)
{
	while (!stack_is_empty(s)) {
		putc(stack_pop(s), out);
	}
}
