#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdint.h>

// Callee-saved registers for x86_64 System V AMD64 ABI:
// rbx, rbp, r12, r13, r14, r15, rsp
typedef struct {
    uint64_t rbx;
    uint64_t rbp;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rsp;
} Context;

void ctx_save(Context* ctx);
void ctx_load(Context* ctx);
void ctx_switch(Context* prev, Context* next);

#endif
