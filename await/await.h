#ifndef AWAIT_H
#define AWAIT_H

#include "../context/context.h"
#include <stdlib.h>
#include <stdint.h>

#define CAT_(a, b) a##b
#define CAT(a, b) CAT_(a, b)

#define DEFAULT_STACK_SIZE 0x10000

#define AWAIT_IMPL(label) do { \
    loop_clean_dead(); \
    Node* _current = loop_current_node(); \
    Node* _next = loop_next_node(); \
    if (_current != _next && _next != NULL) { \
        ctx_switch(&_current->ctx, &_next->ctx); \
    } \
    loop_clean_dead(); \
} while(0)

#define AWAIT() AWAIT_IMPL(CAT(_LABEL, __COUNTER__))

#define ADD_TO_LOOP(func, stack_size) do { \
    loop_push(stack_size); \
    Node* _last = loop_get_last(); \
    uintptr_t _top = ((uintptr_t)_last->stack_mem + (stack_size)) & ~0xFULL; \
    *(uint64_t*)(_top - 8) = (uint64_t)coroutine_exit; \
    *(uint64_t*)(_top - 16) = (uint64_t)(uintptr_t)(func); \
    _last->ctx.rsp = _top - 16; \
    _last->label = (void*)(uintptr_t)(func); \
} while(0)

#endif