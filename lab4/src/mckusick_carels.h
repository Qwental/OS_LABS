//
// Created by Qwental on 22.12.2024.
//

#ifndef MCKUSICK_CARELS_H
#define MCKUSICK_CARELS_H

#include <stddef.h>

#define ALIGN_SIZE(size, alignment) (((size) + (alignment - 1)) & ~(alignment - 1)) // Выравнивание размера
#define FREE_LIST_ALIGNMENT 8

typedef struct Block {
    struct Block *next_block;
} Block;

typedef struct Allocator {
    void *base_addr;
    size_t total_size;
    Block *free_list_head;
} Allocator;

Allocator *allocator_create(void *memory_region, size_t region_size);

void allocator_destroy(Allocator *allocator);

void *allocator_alloc(Allocator *allocator, size_t alloc_size);

void allocator_free(Allocator *allocator, void *memory_block);

#endif // MCKUSICK_CARELS_H
