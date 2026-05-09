/*
 * stack.h — Array-backed stack
 *
 * Algorithms: DFS, expression eval, call-frame unwinding
 * Domain use: kernel/user call stacks, protocol decode, undo stack
 */
#pragma once
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    int   *data;
    int    top;
    size_t capacity;
} Stack;

Stack *stack_create(size_t cap);
void   stack_destroy(Stack *s);
bool   stack_push(Stack *s, int val);
int    stack_pop(Stack *s);
int    stack_peek(const Stack *s);
bool   stack_is_empty(const Stack *s);

/* DFS on adjacency list using explicit stack (no recursion) */
void stack_dfs_iterative(int **adj, int n, int src, bool *visited);

/* Evaluate postfix expression e.g. "3 4 + 2 *" */
int  stack_eval_postfix(const char *expr);

/* Check balanced brackets */
bool stack_balanced_brackets(const char *expr);
