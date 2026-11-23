#ifndef ADD_TO_LOOP_H
#define ADD_TO_LOOP_H

#define ADD_TO_LOOP(func, stack_size, retbuf, is_ready, params) do { \
    _Static_assert(_Generic((is_ready), void*: 1, default: 0), \
        "is_ready must be void* type"); \
    \
    _Static_assert(_Generic((is_ready), void*: 1, volatile _Bool*: 1, default: 0), \
        "is_ready must be _Bool* type"); \
    \
    _Static_assert(_Generic((params), void*: 1, default: 0), \
        "params must be void* type"); \
    \
    loop_push(stack_size, retbuf, is_ready, params); \
    Node* _last = loop_get_last(); \
    uintptr_t _top = ((uintptr_t)_last->stack_mem + (stack_size)) & ~0xFULL; \
    *(uint64_t*)(_top - 8) = (uint64_t)coroutine_exit; \
    *(uint64_t*)(_top - 16) = (uint64_t)(uintptr_t)(func); \
    _last->ctx.rsp = _top - 16; \
} while(0)

#endif