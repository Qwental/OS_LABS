#include "block_2n.h"


Allocator *allocator_create(void *memory_region, const size_t region_size) {
    if (memory_region == NULL || region_size < sizeof(Allocator)) {
        return NULL;
    }

    Allocator *allocator = (Allocator *) memory_region;
    allocator->base_addr = memory_region;
    allocator->total_size = region_size;

    const size_t min_usable = sizeof(Block) + BLOCK_MIN_SIZE;
    const size_t max_block = BLOCK_MAX_SIZE(region_size);
    // колво списков для блоков разных размеров
    allocator->num_lists = (size_t) floor(log2_calc(max_block) / DIVISOR_LOG2);
    // указ на начло массива списков своб блоков
    allocator->free_lists = (Block **) ((char *) memory_region + sizeof(Allocator));
    for (size_t i = 0; i < allocator->num_lists; i++) {
        allocator->free_lists[i] = NULL;
    }

    void *current_block = (char *) memory_region + sizeof(Allocator) +
                          allocator->num_lists * sizeof(Block *);// указ на первый блок памяти
    size_t remaining_size = region_size - sizeof(Allocator) - allocator->num_lists * sizeof(Block *); // факт размер без метаданных

    size_t block_size = BLOCK_MIN_SIZE; // мин размер блока
    while (remaining_size >= min_usable) {
        // выделяем блоки
        if (block_size > remaining_size || block_size > max_block) {

            break;
        }

        if (remaining_size >= (block_size + sizeof(Block)) * 2) {
            // если хватает для двух блоков - мерж блоков
            for (int i = 0; i < 2; i++) {
                Block *block_header = (Block *) current_block;
                size_t list_index = log2_calc(block_size);
                block_header->next_block = allocator->free_lists[list_index];
                allocator->free_lists[list_index] = block_header;

                current_block = (char *) current_block + block_size;// двиг указатель
                remaining_size -= block_size;
            }
        } else {
            // выделяем один блок
            Block *block_header = (Block *) current_block;
            size_t list_index = log2_calc(block_size);
            block_header->next_block = allocator->free_lists[list_index];
            allocator->free_lists[list_index] = block_header;

            current_block = (char *) current_block + remaining_size;// двигаем указ
            remaining_size = 0;
        }

        block_size <<= 1;
    }
    return allocator;
}

void *allocator_alloc(Allocator *allocator, size_t alloc_size) {
    if (allocator == NULL || alloc_size == 0) {
        return NULL;
    }

    size_t list_index = log2_calc(alloc_size) + 1;// опред индекс списка для выделения блока с размеров >= alloc_size
    list_index = SELECT_LAST_LIST(list_index, allocator->num_lists);
    //
    while (list_index < allocator->num_lists && allocator->free_lists[list_index] == NULL) {
        list_index++; // ищем покрупнее
    }

    if (list_index >= allocator->num_lists) {
        return NULL;
        // нельзя выделить память
    }

    Block *allocated_block = allocator->free_lists[list_index]; // указ на аллоцированную память
    allocator->free_lists[list_index] = allocated_block->next_block;

    return (void *) ((char *) allocated_block + sizeof(Block));
}

void allocator_free(Allocator *allocator, void *memory_pointer) {
    if (allocator == NULL || memory_pointer == NULL) {
        return;
    }

    Block *block_to_free = (Block *) ((char *) memory_pointer - sizeof(Block));
    size_t offset = (char *) block_to_free - (char *) allocator->base_addr;
    size_t temp_size = BLOCK_DEFAULT_SIZE;
    // вычисляем размер блока, соотв индексу в списке
    while (temp_size <= offset) {
        size_t next_size = temp_size << 1;
        if (next_size > offset) {
            break;
        }
        temp_size = next_size;
    }

    size_t list_index = log2_calc(temp_size);
    list_index = SELECT_LAST_LIST(list_index, allocator->num_lists);
    // Добавляем блок в список соотв размера
    block_to_free->next_block = allocator->free_lists[list_index];
    allocator->free_lists[list_index] = block_to_free;
}

void allocator_destroy(Allocator *allocator) {
    if (allocator == NULL) {
        return;
    }

    allocator->base_addr = NULL;
    allocator->total_size = 0;
    allocator->free_lists = NULL;
}

int log2_calc(int number) {
    if (number == 0) {
        return -1;
    }
    int log_value = 0;
    while (number > 1) {
        number >>= 1;
        log_value++;
    }
    return log_value;
}
