//
// Created by Qwental on 22.12.2024.
//


#include "mckusick_carels.h"

// Функция для создания аллокатора
MemoryAllocator *allocator_create(void *memory_pool, size_t total_size) {
    if (memory_pool == NULL || total_size < sizeof(MemoryAllocator)) {
        return NULL;
    }

    MemoryAllocator *allocator = (MemoryAllocator *)memory_pool;
    allocator->memory_start = (char *)memory_pool + sizeof(MemoryAllocator);
    allocator->memory_size = total_size - sizeof(MemoryAllocator);
    allocator->free_list_head = (FreeBlock *)allocator->memory_start;

    // Инициализация списка
    if (allocator->free_list_head != NULL) {
        allocator->free_list_head->next_block = NULL;
    }

    return allocator;
}

// Функция для уничтожения аллокатора
void allocator_destroy(MemoryAllocator *allocator) {
    if (allocator == NULL) {
        return;
    }

    allocator->memory_start = NULL;
    allocator->memory_size = 0;
    allocator->free_list_head = NULL;
}

// Функция для выделения памяти
void *allocator_alloc(MemoryAllocator *allocator, size_t size) {
    if (allocator == NULL || size == 0) {
        return NULL;
    }

    size_t aligned_size = ALIGN_SIZE(size, FREE_LIST_ALIGNMENT);
    FreeBlock *previous_block = NULL;
    FreeBlock *current_block = allocator->free_list_head;

    while (current_block != NULL) {
        if (aligned_size <= allocator->memory_size) {
            if (previous_block != NULL) {
                previous_block->next_block = current_block->next_block;
            } else {
                allocator->free_list_head = current_block->next_block;
            }
            return current_block;
        }

        previous_block = current_block;
        current_block = current_block->next_block;
    }

    return NULL;
}

// Функция для освобождения памяти
void allocator_free(MemoryAllocator *allocator, void *memory_block) {
    if (allocator == NULL || memory_block == NULL) {
        return;
    }

    FreeBlock *block_to_free = (FreeBlock *)memory_block;
    block_to_free->next_block = allocator->free_list_head;
    allocator->free_list_head = block_to_free;
}
