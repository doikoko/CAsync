#include "loop.h"
#include <stdlib.h>
#include <stdio.h>

Node* node_create(unsigned int stack_size) {
    Node* node = (Node*)malloc(sizeof(Node));
    if (!node) exit(1);

    node->label = NULL;
    
    // stack not in heap for main thread (stack_size == 0)
    if (stack_size == 0)
        node->stack_mem = NULL;
    else 
        node->stack_mem = malloc(stack_size);

    node->next = NULL;
    node->ctx = (Context){0};

    return node;
}

_Loop loop_create(void) {
    return (_Loop) {
        // first node is the main thread, stack already provided by OS
        .node = node_create(0),
        .dead = NULL,
        .current = 0,
        .size = 1
    };
}

void loop_clean_dead(void) {
    if (loop_ptr && loop_ptr->dead) {
        Node* dead = loop_ptr->dead;
        loop_ptr->dead = NULL;
        if (dead->stack_mem) {
            free(dead->stack_mem);
        }
        free(dead);
    }
}

void loop_delete(_Loop* loop) {
    loop_clean_dead();
    Node* current = loop->node;
    while (current) {
        Node* next = current->next;
        if (current->stack_mem)
            free(current->stack_mem);
        free(current);
        current = next;
    }
    loop->node = NULL;
    loop->size = 0;
    loop->current = 0;
    loop->dead = NULL;
}

Context* loop_get_context(unsigned int index) {
    if (!loop_ptr || index >= loop_ptr->size) return NULL;

    Node* current = loop_ptr->node;
    for (unsigned int i = 0; i < index && current; i++)
        current = current->next;
    
    return current ? &current->ctx : NULL;
}

Node* loop_get_last(void) {
    if (!loop_ptr || !loop_ptr->node) return NULL;

    Node* current = loop_ptr->node;
    while (current->next) {
        current = current->next;
    }

    return current;
}

void loop_push(unsigned int stack_size) { 
    if (!loop_ptr) return;

    if (!loop_ptr->node) {
        loop_ptr->node = node_create(stack_size);
        loop_ptr->size = 1;
        return;
    }

    Node* current = loop_ptr->node;
    while (current->next) {
        current = current->next;
    }
    
    current->next = node_create(stack_size);
    loop_ptr->size++;
}

void loop_remove(unsigned int index) {
    loop_clean_dead();
    if (!loop_ptr || index >= loop_ptr->size) return;
    
    Node* current = loop_ptr->node;
    if (!current) return;
    if (index == 0) {
        loop_ptr->node = current->next;
    } else {
        Node* prev = loop_ptr->node;
        for (unsigned int i = 0; i < index - 1; i++) {
            prev = prev->next;
        }
        current = prev->next;
        prev->next = current->next;
    }
    
    loop_ptr->dead = current;
    loop_ptr->size--;

    if (loop_ptr->current >= loop_ptr->size && loop_ptr->size > 0)
        loop_ptr->current = 0;
    else if (index < loop_ptr->current)
        loop_ptr->current--;
}

void coroutine_exit(void) {
    loop_clean_dead();
    loop_remove(loop_ptr->current); \
    Node* next = loop_current_node(); 
    if (next) {
        ctx_switch(NULL, &next->ctx);
    }
}

Node* loop_next_node(void) {
    if (!loop_ptr || loop_ptr->size == 0) return NULL;

    loop_ptr->current = (loop_ptr->current + 1) % loop_ptr->size;

    Node* current = loop_ptr->node;
    for (unsigned int i = 0; i < loop_ptr->current && current; i++) {
        current = current->next;
    }

    return current;
}

Node* loop_next_context(void) {
    return loop_next_node();
}

Node* loop_current_node(void) {
    if (!loop_ptr || loop_ptr->size == 0) return NULL;

    Node* current = loop_ptr->node;
    for (unsigned int i = 0; i < loop_ptr->current && current; i++) {
        current = current->next;
    }

    return current;
}
