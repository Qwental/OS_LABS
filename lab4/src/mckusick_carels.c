//
// Created by Qwental on 22.12.2024.
//

#include "mckusick_carels.h"

Allocator *allocator_create(void *memory_region, const size_t region_size) {
    if (memory_region == NULL || region_size < sizeof(Allocator)) {
        return NULL;
    }

    Allocator *allocator = (Allocator *) memory_region;
    allocator->base_addr = (char *) memory_region + sizeof(Allocator);
    allocator->total_size = region_size - sizeof(Allocator);
    allocator->free_list_head = (Block *) allocator->base_addr;

    // Инициализация списка
    if (allocator->free_list_head != NULL) {
        allocator->free_list_head->next_block = NULL;
    }

    return allocator;
}



void *allocator_alloc(Allocator *allocator, size_t alloc_size) {
    if (allocator == NULL || alloc_size == 0) {
        return NULL;
    }

    size_t aligned_size = ALIGN_SIZE(alloc_size, FREE_LIST_ALIGNMENT);
    Block *previous_block = NULL;
    Block *current_block = allocator->free_list_head;

    while (current_block != NULL) {
        if (aligned_size <= allocator->total_size) { // достаточно ли места
            if (previous_block != NULL) {
                previous_block->next_block = current_block->next_block;
            } else {
                allocator->free_list_head = current_block->next_block;
            }
            return current_block; // возвращаем найденный блок
        }

        previous_block = current_block;
        current_block = current_block->next_block;
    }

    return NULL; // не смогли выделить память
}

void allocator_free(Allocator *allocator, void *memory_block) {
    if (allocator == NULL || memory_block == NULL) {
        return;
    }

    Block *block_to_free = (Block *) memory_block;
    block_to_free->next_block = allocator->free_list_head;
    allocator->free_list_head = block_to_free; // Добавление блока обратно в список свободных блоков
}

void allocator_destroy(Allocator *allocator) {
    if (allocator == NULL) {
        return;
    }

    allocator->base_addr = NULL;
    allocator->total_size = 0;
    allocator->free_list_head = NULL;
}