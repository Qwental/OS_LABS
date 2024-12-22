#ifndef McKusick_Carels_algorithm_H
#define McKusick_Carels_algorithm_H

#include "allocator.h"

typedef struct FreeBlock {
    struct FreeBlock* nextBlock;
} FreeBlock;

#define ALIGN_SIZE(size, alignment) (((size) + (alignment - 1)) & ~(alignment - 1))

Allocator* allocator_create(void* const memoryPool, const size_t totalSize) {
    if (!memoryPool || totalSize < sizeof(Allocator)) {
        return NULL;
    }

    Allocator* allocator = (Allocator*)memoryPool;
    allocator->memoryStart = (char*)memoryPool + sizeof(Allocator);
    allocator->memorySize = totalSize - sizeof(Allocator);
    allocator->freeList = allocator->memoryStart;

    FreeBlock* initialBlock = (FreeBlock*)allocator->memoryStart;
    initialBlock->nextBlock = NULL;

    return allocator;
}

void allocator_destroy(Allocator* const allocator) {
    if (!allocator) {
        return;
    }
    allocator->memoryStart = NULL;
    allocator->memorySize = 0;
    allocator->freeList = NULL;
}

void* allocator_alloc(Allocator* const allocator, const size_t requestedSize) {
    if (!allocator || requestedSize == 0) {
        return NULL;
    }

    size_t alignedSize = ALIGN_SIZE(requestedSize, sizeof(void*));
    FreeBlock* previousBlock = NULL;
    FreeBlock* currentBlock = (FreeBlock*)allocator->freeList;

    while (currentBlock) {
        if (alignedSize <= allocator->memorySize) {
            if (previousBlock) {
                previousBlock->nextBlock = currentBlock->nextBlock;
            } else {
                allocator->freeList = currentBlock->nextBlock;
            }
            return currentBlock;
        }

        previousBlock = currentBlock;
        currentBlock = currentBlock->nextBlock;
    }

    return NULL;
}

void allocator_free(Allocator* const allocator, void* const memoryBlock) {
    if (!allocator || !memoryBlock) {
        return;
    }

    FreeBlock* blockToFree = (FreeBlock*)memoryBlock;
    blockToFree->nextBlock = (FreeBlock*)allocator->freeList;
    allocator->freeList = blockToFree;
}

#endif //  McKusick_Carels_algorithm
