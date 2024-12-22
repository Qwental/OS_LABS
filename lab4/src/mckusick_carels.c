#ifndef McKusick_Carels_algorithm_H
#define McKusick_Carels_algorithm_H

#include <stddef.h>

typedef struct Allocator {
    void *memory_start;
    size_t memory_size;
    void *free_list;
} Allocator;

Allocator *allocator_create(void *const memory, const size_t size);

void allocator_destroy(Allocator *const allocator);

void *allocator_alloc(Allocator *const allocator, const size_t size);

void allocator_free(Allocator *const allocator, void *const memory);


typedef struct FreeBlock {
    struct FreeBlock *nextBlock;
} FreeBlock;

#define ALIGN_SIZE(size, alignment) (((size) + (alignment - 1)) & ~(alignment - 1))

Allocator *allocator_create(void *const memoryPool, const size_t totalSize) {
    if (!memoryPool || totalSize < sizeof(Allocator)) {
        return NULL;
    }

    Allocator *allocator = (Allocator *) memoryPool;
    allocator->memory_start = (char *) memoryPool + sizeof(Allocator);
    allocator->memory_size = totalSize - sizeof(Allocator);
    allocator->free_list = allocator->memory_start;

    FreeBlock *initialBlock = (FreeBlock *) allocator->memory_start;
    initialBlock->nextBlock = NULL;

    return allocator;
}

void allocator_destroy(Allocator *const allocator) {
    if (!allocator) {
        return;
    }
    allocator->memory_start = NULL;
    allocator->memory_size = 0;
    allocator->free_list = NULL;
}

void *allocator_alloc(Allocator *const allocator, const size_t requestedSize) {
    if (!allocator || requestedSize == 0) {
        return NULL;
    }

    size_t alignedSize = ALIGN_SIZE(requestedSize, sizeof(void*));
    FreeBlock *previousBlock = NULL;
    FreeBlock *currentBlock = (FreeBlock *) allocator->free_list;

    while (currentBlock) {
        if (alignedSize <= allocator->memory_size) {
            if (previousBlock) {
                previousBlock->nextBlock = currentBlock->nextBlock;
            } else {
                allocator->free_list = currentBlock->nextBlock;
            }
            return currentBlock;
        }

        previousBlock = currentBlock;
        currentBlock = currentBlock->nextBlock;
    }

    return NULL;
}

void allocator_free(Allocator *const allocator, void *const memoryBlock) {
    if (!allocator || !memoryBlock) {
        return;
    }

    FreeBlock *blockToFree = (FreeBlock *) memoryBlock;
    blockToFree->nextBlock = (FreeBlock *) allocator->free_list;
    allocator->free_list = blockToFree;
}

#endif //  McKusick_Carels_algorithm
