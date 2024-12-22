//
// Created by Qwental on 22.12.2024.
//

#ifndef MCKUSICK_CARELS_H
#define MCKUSICK_CARELS_H

#include <stddef.h>

// Макросы для выравнивания
#define ALIGN_SIZE(size, alignment) (((size) + (alignment - 1)) & ~(alignment - 1)) // Выравнивание размера
#define FREE_LIST_ALIGNMENT 8 // Выравнивание списка свободных блоков

// Структуры
typedef struct FreeBlock {
    struct FreeBlock *next_block;
} FreeBlock;

typedef struct MemoryAllocator {
    void *memory_start;
    size_t memory_size;
    FreeBlock *free_list_head;
} MemoryAllocator;

// Объявления функций
MemoryAllocator *allocator_create(void *memory_pool, size_t total_size);

void allocator_destroy(MemoryAllocator *allocator);

void *allocator_alloc(MemoryAllocator *allocator, size_t size);

void allocator_free(MemoryAllocator *allocator, void *memory_block);

#endif // MCKUSICK_CARELS_H
