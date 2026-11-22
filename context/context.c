#include "context.h"

void ctx_save(Context* ctx) {
    __asm__ __volatile__ (
        "movq %%rbx, %0\n\t"
        "movq %%rbp, %1\n\t"
        "movq %%r12, %2\n\t"
        "movq %%r13, %3\n\t"
        "movq %%r14, %4\n\t"
        "movq %%r15, %5\n\t"
        "movq %%rsp, %6\n\t"
        : "=m"(ctx->rbx),
          "=m"(ctx->rbp),
          "=m"(ctx->r12),
          "=m"(ctx->r13),
          "=m"(ctx->r14),
          "=m"(ctx->r15),
          "=m"(ctx->rsp)
        :
        : "memory"
    );
}

void ctx_load(Context* ctx) {
    __asm__ __volatile__ (
        "movq %0, %%rbx\n\t"
        "movq %1, %%rbp\n\t"
        "movq %2, %%r12\n\t"
        "movq %3, %%r13\n\t"
        "movq %4, %%r14\n\t"
        "movq %5, %%r15\n\t"
        "movq %6, %%rsp\n\t"
        :
        : "m"(ctx->rbx),
          "m"(ctx->rbp),
          "m"(ctx->r12),
          "m"(ctx->r13),
          "m"(ctx->r14),
          "m"(ctx->r15),
          "m"(ctx->rsp)
        : "memory"
    );
}

__attribute__((naked))
void ctx_switch(Context* prev, Context* next) {
    (void)prev;
    (void)next;
    __asm__ __volatile__ (
        // if prev is not NULL, save callee-saved registers and stack pointer
        "testq %rdi, %rdi\n\t"
        "jz 1f\n\t"
        "movq %rbx, 0(%rdi)\n\t"
        "movq %rbp, 8(%rdi)\n\t"
        "movq %r12, 16(%rdi)\n\t"
        "movq %r13, 24(%rdi)\n\t"
        "movq %r14, 32(%rdi)\n\t"
        "movq %r15, 40(%rdi)\n\t"
        "movq %rsp, 48(%rdi)\n\t"

        "1:\n\t"
        // restore next callee-saved registers and stack pointer
        "movq 0(%rsi), %rbx\n\t"
        "movq 8(%rsi), %rbp\n\t"
        "movq 16(%rsi), %r12\n\t"
        "movq 24(%rsi), %r13\n\t"
        "movq 32(%rsi), %r14\n\t"
        "movq 40(%rsi), %r15\n\t"
        "movq 48(%rsi), %rsp\n\t"
        "ret\n\t"
    );
}