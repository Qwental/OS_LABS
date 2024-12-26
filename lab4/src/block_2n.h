#ifndef BLOCK_2N_H
#define BLOCK_2N_H

#include <math.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

//  вычисляет указатель на список свободных блоков по индексу в структуре аллокатора.
#define OFFSET_FREE_LIST(index, allocator) ((Block **) ((char *)(allocator) + sizeof(Allocator)))[index]
#define BLOCK_MIN_SIZE 16
#define BLOCK_DEFAULT_SIZE 32
#define DIVISOR_LOG2 2

#define BLOCK_MAX_SIZE(size) (((size) < BLOCK_DEFAULT_SIZE) ? BLOCK_DEFAULT_SIZE : (size))
//Если индекс превышает количество списков, возвращается индекс последнего списка.
#define SELECT_LAST_LIST(index, num_lists) (((index) >= (num_lists)) ? ((num_lists) - 1) : (index))

typedef struct Block {
    struct Block *next_block;
} Block;

typedef struct Allocator {
    Block **free_lists; // yказ на своб блоки разного разм
    size_t num_lists;
    void *base_addr; //yказ на начало выдел пямяти
    size_t total_size;
} Allocator;

int log2_calc(int number);
Allocator *allocator_create(void *memory_region, size_t region_size);
void *allocator_alloc(Allocator *allocator, size_t alloc_size);
void allocator_free(Allocator *allocator, void *memory_pointer);
void allocator_destroy(Allocator *allocator);

#endif // BLOCK_2N_H