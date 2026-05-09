/*
 * stack.c — Array-backed stack with DFS, postfix eval, bracket matching
 */
#include "linear/stack.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

Stack *stack_create(size_t cap) {
    Stack *s = malloc(sizeof *s);
    s->data = malloc(cap * sizeof(int));
    s->top = -1; s->capacity = cap;
    return s;
}

void stack_destroy(Stack *s) { free(s->data); free(s); }

bool stack_push(Stack *s, int val) {
    if ((size_t)(s->top + 1) >= s->capacity) return false;
    s->data[++s->top] = val; return true;
}

int  stack_pop(Stack *s)              { assert(s->top>=0); return s->data[s->top--]; }
int  stack_peek(const Stack *s)       { assert(s->top>=0); return s->data[s->top]; }
bool stack_is_empty(const Stack *s)   { return s->top < 0; }

/*
 * Iterative DFS using explicit stack — avoids recursion stack overflow
 * on deep graphs (kernel module dependency graphs can be deep)
 * Complexity: O(V + E)
 */
void stack_dfs_iterative(int **adj, int n, int src, bool *visited) {
    Stack *st = stack_create((size_t)n);
    stack_push(st, src);
    while (!stack_is_empty(st)) {
        int v = stack_pop(st);
        if (visited[v]) continue;
        visited[v] = true;
        printf("DFS visit: %d\n", v);
        /* push neighbours (adj[v][0] = count, adj[v][1..] = neighbours) */
        for (int i = 1; i <= adj[v][0]; i++)
            if (!visited[adj[v][i]])
                stack_push(st, adj[v][i]);
    }
    stack_destroy(st);
}

/*
 * Evaluate postfix expression — e.g. "3 4 + 2 *" → 14
 * Algorithm: scan tokens; push numbers, pop & apply operators
 * Domain: Linux BPF bytecode evaluation, expression engines
 */
int stack_eval_postfix(const char *expr) {
    Stack *s = stack_create(64);
    const char *p = expr;
    int result = 0;
    while (*p) {
        while (*p == ' ') p++;
        if (isdigit(*p)) {
            int num = 0;
            while (isdigit(*p)) num = num*10 + (*p++ - '0');
            stack_push(s, num);
        } else if (*p == '+' || *p == '-' || *p == '*' || *p == '/') {
            int b = stack_pop(s), a = stack_pop(s);
            switch (*p) {
                case '+': stack_push(s, a + b); break;
                case '-': stack_push(s, a - b); break;
                case '*': stack_push(s, a * b); break;
                case '/': stack_push(s, a / b); break;
            }
            p++;
        } else p++;
    }
    result = stack_pop(s);
    stack_destroy(s);
    return result;
}

/* Balanced bracket check — O(n) */
bool stack_balanced_brackets(const char *expr) {
    Stack *s = stack_create(strlen(expr));
    for (const char *p = expr; *p; p++) {
        if (*p=='('||*p=='['||*p=='{') stack_push(s, *p);
        else if (*p==')'||*p==']'||*p=='}') {
            if (stack_is_empty(s)) { stack_destroy(s); return false; }
            int top = stack_pop(s);
            if ((*p==')' && top!='(') ||
                (*p==']' && top!='[') ||
                (*p=='}' && top!='{')) { stack_destroy(s); return false; }
        }
    }
    bool ok = stack_is_empty(s);
    stack_destroy(s);
    return ok;
}
