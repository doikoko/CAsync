#ifndef JOIN_H
#define JOIN_H

#define JOIN(is_ready) do { \
    _Static_assert(_Generic((is_ready), _Bool: 1, default: 0), \
        "is_ready must be _Bool* type"); \
    \
    while(!is_ready) AWAIT(); \
} while(0)
#endif