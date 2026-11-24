#ifndef FUTURE_H
#define FUTURE_H

#include "../context/context.h"

typedef struct Node {
    Context ctx;
    void* retbuf;
    bool *is_ready;
    void* params;
    void* stack_mem;
    struct Node* next;
} Node;

// linked list circular event loop
typedef struct {
    Node* node;
    Node* dead;
    unsigned int current;
    unsigned int size;
} _Loop; // you shouldn't use this definition directly. Use Loop macro

#define Loop __attribute__((cleanup(loop_delete))) _Loop
#define LOOP_INIT() \
    Loop loop = loop_create(); \
    loop_ptr = &loop;

_Loop loop_create(void);
void loop_delete(_Loop* loop);
void loop_clean_dead(void);
Node* loop_current_node(void);
Node* loop_next_node(void);
Node* loop_next_context(void);
Node* loop_get_last(void);
Context* loop_get_context(unsigned int index);
void loop_push(unsigned int stack_size, void* retbuf, _Bool* is_ready, void* params);
void loop_remove(unsigned int index);
void coroutine_exit(void);

extern _Loop* loop_ptr;

#endif
