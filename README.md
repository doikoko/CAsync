# CAsync

A lightweight, stackful coroutine and cooperative multitasking library implemented in pure C (C11) for x86_64 architecture (System V AMD64 ABI).

`CAsync` provides fiber-like execution contexts and an intuitive round-robin event loop using direct assembly register switching without heavy external dependencies.

---

## Features

- **Cooperative Multitasking**: Non-preemptive scheduling where coroutines voluntarily yield execution via `AWAIT()`.
- **Stackful Coroutines**: Each coroutine receives its own dedicated stack allocation with customized stack sizes.
- **Fast Assembly Context Switching**: Saves and restores System V AMD64 callee-saved registers (`rbx`, `rbp`, `r12`-`r15`, `rsp`) directly via inline GNU assembly.
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
- Each spawned coroutine points to a heap-allocated stack.
- `coroutine_exit()` is automatically placed onto the coroutine return address so exiting functions automatically unregister themselves from the active loop and trigger stack reclamation.

### 3. Yielding & Scheduling (`await/`)
The `AWAIT()` macro pauses the currently running coroutine, rotates to the next runnable node in the queue, performs a context switch, and purges finished tasks.

---

## Prerequisites & Compatibility

- **OS**: Linux / POSIX-compliant x86_64 OS
- **Compiler**: GCC or Clang supporting C11 and GNU inline assembly
- **Architecture**: x86_64 (AMD64 System V ABI)
- **Build System**: CMake >= 3.28 (optional, can also compile directly)

---

## Code Example

The following complete example demonstrates initializing the loop, registering multiple coroutines with custom stack sizes, yielding control using `AWAIT()`, and resuming execution across tasks:

```c
#include "CAsync/async.h"
#include <stdio.h>

_Loop* loop_ptr = NULL;

void func(void){
    printf("func\n");
    AWAIT();
    int a = 4; // stack usage demonstation
    printf("%d\n", a);
}

void func2(void){
    printf("func2\n");
    AWAIT();
    printf("func2 next\n");
}

int main(void){
    LOOP_INIT();
    
    // 0x4000 - stack size
    ADD_TO_LOOP(func, 0x4000);
    ADD_TO_LOOP(func2, 0x4000);
    printf("main\n");
    AWAIT();
    
    printf("end\n");
    AWAIT();

    return 0;
}
```

### Expected Output

```text
main
func
func2
end
4
func2 next
```

### Execution Trace

1. `main` registers `func` and `func2` onto the event loop queue and prints `main`.
2. `main` calls `AWAIT()`, switching context to `func`.
3. `func` prints `func` and yields via `AWAIT()`, passing execution to `func2`.
4. `func2` prints `func2` and yields via `AWAIT()`, returning control back to `main`.
5. `main` prints `end` and yields via `AWAIT()`, resuming `func` right after its yield point.
6. `func` prints `4` and returns, implicitly triggering `coroutine_exit()`.
7. `func2` resumes, prints `func2 next`, and terminates.
8. Execution finishes cleanly as all active coroutines complete.

---

## API Reference

| Macro / Function | Description |
| :--- | :--- |
| `LOOP_INIT()` | Instantiates a scope-managed event loop and assigns `loop_ptr`. |
| `ADD_TO_LOOP(func, stack_size)` | Allocates an execution node and stack, preparing `func` as a coroutine. |
| `AWAIT()` | Yields execution of the current coroutine to the next scheduled node. |
| `DEFAULT_STACK_SIZE` | Default stack allocation constant (`0x10000`, 64 KB). |
