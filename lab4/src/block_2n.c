//
// Created by Qwental on 22.12.2024.
//
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

#define MAX_BLOCK_SIZE_LOG2 10   // Максимальный логарифм размера блока (2^10 = 1024)
#define MIN_BLOCK_SIZE_LOG2 0    // Минимальный логарифм размера блока (2^0 = 1)
#define FREE_LIST_COUNT 11       // Количество списков свободных блоков (0-10)

typedef struct Block {
    struct Block *next;
    struct Block *prev;
    size_t size;
} Block;

typedef struct Allocator {
    Block *freeLists[FREE_LIST_COUNT];
    void *memoryStart;
    size_t totalMemorySize;
} Allocator;

// Вычисление степени числа (base^exp)
long long calculate_power(int base, int exp) {
    long long result = 1;
    while (exp > 0) {
        if (exp & 1) {
            result *= base;
        }
        base *= base;
        exp /= 2;
    }
    return result;
}

// Создание аллокатора
Allocator *allocator_create(void *memoryPool, const size_t totalSize) {
    Allocator *allocator = (Allocator *) memoryPool;
    allocator->totalMemorySize = totalSize - sizeof(Allocator);
    allocator->memoryStart = (char *) memoryPool + sizeof(Allocator);

    size_t offset = 0;
    size_t currentBlockLog = 0;

    // Инициализация списков свободных блоков
    for (int i = 0; i < FREE_LIST_COUNT; ++i) {
        allocator->freeLists[i] = NULL;
    }

    // Разбиение памяти на блоки
    while (offset + calculate_power(2, currentBlockLog) <= allocator->totalMemorySize) {
        Block *block = (Block *) ((char *) allocator->memoryStart + offset);
        block->size = calculate_power(2, currentBlockLog);

        // Добавление блока в соответствующий список
        block->next = allocator->freeLists[currentBlockLog];
        block->prev = NULL;
        if (allocator->freeLists[currentBlockLog]) {
            allocator->freeLists[currentBlockLog]->prev = block;
        }
        allocator->freeLists[currentBlockLog] = block;

        offset += block->size;
        currentBlockLog++;
    }

    return allocator;
}

// Разделение блока
void split_block(Allocator *allocator, Block *block) {
    int logIndex = 0;
    while ((1 << logIndex) < block->size) {
        logIndex++;
    }

    Block *newBlock = (Block *) ((char *) allocator->memoryStart + block->size / 2);

    // Настройка ссылок
    newBlock->next = allocator->freeLists[logIndex];
    if (newBlock->next) {
        newBlock->next->prev = newBlock;
    }
    allocator->freeLists[logIndex] = newBlock;

    block->size /= 2;
    newBlock->size = block->size;
}

// Выделение памяти
void *allocator_alloc(Allocator *allocator, const size_t requestedSize) {
    int logIndex = 0;

    // Поиск подходящего размера блока
    while ((1 << logIndex) < requestedSize) {
        logIndex++;
    }

    if (logIndex >= FREE_LIST_COUNT) {
        return NULL; // Запрашиваемый размер слишком велик
    }

    // Поиск свободного блока
    for (int i = logIndex; i < FREE_LIST_COUNT; ++i) {
        if (allocator->freeLists[i] != NULL) {
            Block *block = allocator->freeLists[i];

            // Удаление из списка и разбиение блока, если требуется
            allocator->freeLists[i] = block->next;
            while (i > logIndex) {
                split_block(allocator, block);
                i--;
            }

            return (void *) block;
        }
    }

    return NULL; // Нет доступных блоков
}

// Освобождение памяти
void allocator_free(Allocator *allocator, void *memoryBlock) {
    if (!allocator || !memoryBlock) {
        return;
    }

    Block *block = (Block *) memoryBlock;
    int logIndex = 0;

    // Определение индекса списка
    while ((1 << logIndex) < block->size) {
        logIndex++;
    }

    // Добавление блока обратно в список
    block->next = allocator->freeLists[logIndex];
    if (block->next) {
        block->next->prev = block;
    }
    allocator->freeLists[logIndex] = block;
}

// Уничтожение аллокатора
void allocator_destroy(Allocator *allocator) {
    if (!allocator) {
        return;
    }

    if (munmap((void *) allocator, allocator->totalMemorySize + sizeof(Allocator)) == -1) {
        exit(EXIT_FAILURE); // Ошибка освобождения памяти
    }
}
