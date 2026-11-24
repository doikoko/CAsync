# CAsync

A lightweight, stackful coroutine and cooperative multitasking library implemented in pure C for x86_64 architecture (System V AMD64 ABI).

`CAsync` provides fiber-like execution contexts, argument passing, return values, task synchronization (`JOIN`), and an intuitive round-robin event loop using direct assembly register switching without heavy external dependencies.

---

## Features

- **Cooperative Multitasking**: Non-preemptive scheduling where coroutines voluntarily yield execution via `AWAIT()`.
- **Stackful Coroutines**: Each coroutine receives its own dedicated stack allocation with customized stack sizes.
- **Fast Assembly Context Switching**: Saves and restores System V AMD64 callee-saved registers (`rbx`, `rbp`, `r12`-`r15`, `rsp`) directly via inline GNU assembly.
- **Parameters & Return Values**: Pass arguments sequentially to coroutines and retrieve typed results via caller-allocated buffers.
- **Synchronization (`JOIN`)**: Efficiently wait for coroutine completion using status flags without blocking the event loop.
- **Circular Event Loop**: Linked-list event loop managing active coroutines and dynamically recycling terminated tasks.
- **Automatic Resource Cleanup**: Uses GCC/Clang `__attribute__((cleanup))` to guarantee loop memory deallocation upon exiting scope.

---

## Architecture & Internals

### 1. Context (`context/`)
The `Context` structure stores the CPU registers that must be preserved across function calls according to the System V AMD64 ABI:
- Callee-saved: `rbx`, `rbp`, `r12`, `r13`, `r14`, `r15`
- Stack pointer: `rsp`

`ctx_switch(prev, next)` swaps the register state and stack pointer between two execution contexts using naked assembly routines.

### 2. Event Loop (`loop/`)
Coroutines reside in an execution ring represented by `Node` structures:
- The initial node represents the main program thread using the OS-provided stack.
- Each spawned coroutine points to a heap-allocated stack (`stack_mem`).
- Each node stores pointers for return values (`retbuf`), readiness notification (`is_ready`), and argument streaming (`params`).
- `coroutine_exit()` is placed onto the coroutine return address so exiting functions automatically unregister themselves from the active loop and trigger stack reclamation.

### 3. Yielding & Scheduling (`await/`)
The `AWAIT()` macro pauses the currently running coroutine, rotates to the next runnable node in the queue, performs a context switch, and purges finished tasks.

---

## Passing Parameters & Returning Values

### 1. Passing Parameters (`get_param/`)
Parameters can be packed into a contiguous memory buffer (such as a struct) and passed to `ADD_TO_LOOP`.

Inside the coroutine, retrieve arguments in the same order they are packed using `GET_PARAM(type, node)`:

```c
Node* current = loop_current_node();

void *__restrict ptr = GET_PARAM(void *__restrict, current);
size_t size          = GET_PARAM(size_t, current);
size_t n             = GET_PARAM(size_t, current);
FILE* stream         = GET_PARAM(FILE*, current);
```
`GET_PARAM` casts the current pointer to `type*`, dereferences it, and advances `node->params` by `sizeof(type)`.

### 2. Returning Values (`a_ret/`)
To avoid dynamic heap allocations, the caller allocates a return variable (on its stack or heap) and passes its address `&ret` to `ADD_TO_LOOP`.

When the coroutine completes, it assigns the result using `A_RET(val)`:

```c
A_RET(count);
```

`A_RET` writes the value to `loop_current_node()->retbuf` and sets the `is_ready` flag to `1`. Coroutines returning values can optionally use readability typedefs such as `LONG func(void)` or `INT func(void)`.

### 3. Synchronization (`join/`)
The caller waits for one or more coroutines to finish with `JOIN(is_ready)`:

```c
volatile _Bool is_ready1 = 0;
// ...
JOIN(is_ready1);
```
`JOIN` checks the readiness flag and repeatedly yields with `AWAIT()` until the task completes.

---

## Prerequisites & Compatibility

- **OS**: Linux / POSIX-compliant x86_64 OS
- **Compiler**: GCC or Clang supporting C11 (or C23) and GNU inline assembly
- **Architecture**: x86_64 (AMD64 System V ABI)
- **Build System**: CMake >= 3.28 (optional, can also compile directly)

---

## Code Example

The following example demonstrates asynchronous parallel file reading across multiple coroutines with parameter passing, cooperative yielding (`AWAIT`), return values (`A_RET`), and waiting with `JOIN`:

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CAsync/join/join.h"
#include "CAsync/loop/loop.h"
#include "CAsync/await/await.h"
#include "CAsync/add_to_loop/add_to_loop.h"
#include "CAsync/a_ret/a_ret.h"
#include "CAsync/get_param/get_param.h"

_Loop* loop_ptr = NULL;

#define ITERS 5

LONG a_fread(void) {
    Node* current = loop_current_node();
    
    void *__restrict ptr = GET_PARAM(void *__restrict, current);
    size_t size = GET_PARAM(size_t, current);
    size_t n = GET_PARAM(size_t, current);
    FILE* stream = GET_PARAM(FILE*, current);

    unsigned long count = 0;

    for (uint8_t i = 0; i < ITERS; i++) {
        count += fread((char*)ptr + count * size, size, n / ITERS, stream);
        AWAIT();
    }

    size_t remaining = n % ITERS;
    if (remaining) {
        count += fread((char*)ptr + count * size, size, remaining, stream);
    }

    A_RET(count);
}

size_t file_size(FILE* f) {
    if (fseek(f, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(f);
        exit(1);
    }

    long size = ftell(f);
    if (size < 0) {
        perror("ftell");
        fclose(f);
        exit(1);
    }

    fseek(f, 0, SEEK_SET);
    return (size_t)size;
}

#define BUF_SIZE 1000

typedef struct {
    void* ptr;
    size_t size;
    size_t n;
    FILE* stream;
} FreadParams;

int main(void) {
    LOOP_INIT();

    char* buf1 = malloc(BUF_SIZE);
    char* buf2 = malloc(BUF_SIZE);

    FILE* f1 = fopen("1.txt", "r");
    FILE* f2 = fopen("2.txt", "r");

    if (!f1 || !f2) {
        perror("fopen");
        free(buf1);
        free(buf2);
        if (f1) fclose(f1);
        if (f2) fclose(f2);
        return 1;
    }

    size_t n1 = file_size(f1);
    size_t n2 = file_size(f2);

    if (n1 > BUF_SIZE) n1 = BUF_SIZE;
    if (n2 > BUF_SIZE) n2 = BUF_SIZE;

    FreadParams* params1 = malloc(sizeof(FreadParams));
    *params1 = (FreadParams){
        .ptr = buf1,
        .size = sizeof(char),
        .n = n1,
        .stream = f1
    };

    FreadParams* params2 = malloc(sizeof(FreadParams));
    *params2 = (FreadParams){
        .ptr = buf2,
        .size = sizeof(char),
        .n = n2,
        .stream = f2
    };

    size_t ret1 = 0;
    size_t ret2 = 0;

    volatile _Bool is_ready1 = 0;
    volatile _Bool is_ready2 = 0;

    ADD_TO_LOOP(
        a_fread,
        DEFAULT_STACK_SIZE,
        &ret1,
        (void*)&is_ready1,
        (void*)params1
    );

    ADD_TO_LOOP(
        a_fread,
        DEFAULT_STACK_SIZE,
        &ret2,
        (void*)&is_ready2,
        (void*)params2
    );

    JOIN(is_ready1);
    JOIN(is_ready2);

    printf("res1: %zu bytes read\n", ret1);
    printf("res2: %zu bytes read\n", ret2);

    fclose(f1);
    fclose(f2);
    free(params1);
    free(params2);
    free(buf1);
    free(buf2);

    return 0;
}
```

---

## API Reference

| Macro / Function | Description |
| :--- | :--- |
| `LOOP_INIT()` | Instantiates a scope-managed event loop and assigns `loop_ptr`. |
| `ADD_TO_LOOP(func, stack_size, retbuf, is_ready, params)` | Enqueues a coroutine with stack allocation, result target buffer, ready flag, and arguments. |
| `AWAIT()` | Yields execution of the current coroutine to the next scheduled node. |
| `GET_PARAM(type, node)` | Reads an argument of type `type` from `node->params` and advances the pointer. |
| `A_RET(val)` | Writes `val` into caller's `retbuf` and sets `is_ready` to `1`. |
| `JOIN(is_ready)` | Loops with `AWAIT()` until the coroutine sets `is_ready` to `1`. |
| `DEFAULT_STACK_SIZE` | Default stack allocation constant (`0x10000`, 64 KB). |
