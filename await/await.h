#ifndef AWAIT_H
#define AWAIT_H

#include "../context/context.h"
#include <stdlib.h>
#include <stdint.h>

#define CAT_(a, b) a##b
#define CAT(a, b) CAT_(a, b)

#define DEFAULT_STACK_SIZE 0x4000

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

#endif