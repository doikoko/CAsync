#ifndef A_RET_H
#define A_RET_H

// for more readable function defenitions
// e.g. INT func(){}
#ifndef INT
#define INT void
#endif

#ifndef CHAR
#define CHAR void
#endif

#ifndef FLOAT
#define FLOAT void
#endif

#ifndef DOUBLE
#define DOUBLE void
#endif

#ifndef LONG
#define LONG void
#endif

#define A_RET(_val) do { \
    void* _retbuf = loop_current_node()->retbuf; \
    volatile _Bool* is_ready = loop_current_node()->is_ready; \
    if (_retbuf) *((typeof(_val)*)_retbuf) = _val; \
    if (is_ready) *is_ready = 1; \
} while(0)

#endif