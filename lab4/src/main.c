//
// Created by Qwental on 22.12.2024.
//
/*

gcc -shared -fPIC -o libmckusick_carels.so mckusick_carels.c

gcc -shared -fPIC -o libblock_2n.so block_2n.c

gcc -o main main.c -ldl


./main ./libblock_2n.so
./main ./libmckusick_carels.so


 */


#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>

typedef void * (*allocator_create_t)(void *const memory, const size_t size);

typedef void * (*allocator_alloc_t)(void *const allocator, const size_t size);

typedef void (*allocator_free_t)(void *const allocator, void *const memory);

typedef void (*allocator_destroy_t)(void *const allocator);

void write_message(const char *message) {
    write(STDOUT_FILENO, message, strlen(message));
}

void write_error(const char *message) {
    write(STDERR_FILENO, message, strlen(message));
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        write_error("Usage: <program> <path_to_allocator_library>\n");
        return 52;
    }

    // Загружаем динамическую библиотеку
    void *allocator_lib = dlopen(argv[1], RTLD_LAZY);
    if (allocator_lib == NULL) {
        write_error("Error loading library: ");
        write_error(dlerror());
        write_error("\n");
        return 1;
    }

    // Получаем указатели на функции из библиотеки
    allocator_create_t allocator_create = (allocator_create_t) dlsym(allocator_lib, "allocator_create");
    allocator_alloc_t allocator_alloc = (allocator_alloc_t) dlsym(allocator_lib, "allocator_alloc");
    allocator_free_t allocator_free = (allocator_free_t) dlsym(allocator_lib, "allocator_free");
    allocator_destroy_t allocator_destroy = (allocator_destroy_t) dlsym(allocator_lib, "allocator_destroy");

    if (!allocator_create || !allocator_alloc || !allocator_free || !allocator_destroy) {
        write_error("Error locating functions: ");
        write_error(dlerror());
        write_error("\n");
        dlclose(allocator_lib);
        return 1;
    }

    // Выделяем память для аллокатора
    void *memory_pool = malloc(1024 * 1024);
    if (!memory_pool) {
        write_error("Memory allocation for pool failed\n");
        dlclose(allocator_lib);
        return 1;
    }

    // Создаем аллокатор
    void *allocator = allocator_create(memory_pool, 1024 * 1024);
    if (!allocator) {
        write_error("Allocator creation failed\n");
        free(memory_pool);
        dlclose(allocator_lib);
        return 1;
    }

    // Тест 1: выделение 256 байт
    void *block1 = allocator_alloc(allocator, 256);
    if (block1) {
        write_message("Test 1: Memory allocated (256 bytes)\n");
        allocator_free(allocator, block1);
        write_message("Test 1: Memory freed (256 bytes)\n");
    } else {
        write_error("Test 1: Memory allocation failed\n");
    }

    // Тест 2: выделение больше доступного
    void *block2 = allocator_alloc(allocator, 2 * 1024 * 1024);
    if (!block2) {
        write_message("Test 2: Memory allocation failed as expected for oversized request\n");
    } else {
        write_error("Test 2: Unexpected success in oversized allocation\n");
        allocator_free(allocator, block2);
    }

    // Тест 3: повторное выделение и освобождение
    void *block3 = allocator_alloc(allocator, 512);
    if (block3) {
        write_message("Test 3: Memory allocated (512 bytes)\n");
        allocator_free(allocator, block3);
        write_message("Test 3: Memory freed (512 bytes)\n");
    } else {
        write_error("Test 3: Memory allocation failed\n");
    }

    // Тест 4: проверка выделения после освобождения
    void *block4 = allocator_alloc(allocator, 256);
    if (block4) {
        write_message("Test 4: Memory allocated (256 bytes)\n");
        allocator_free(allocator, block4);
        write_message("Test 4: Memory freed (256 bytes)\n");
    } else {
        write_error("Test 4: Memory allocation failed\n");
    }

    // Уничтожаем аллокатор
    allocator_destroy(allocator);
    write_message("Allocator destroyed\n");

    free(memory_pool);
    dlclose(allocator_lib);

    return 0;
}
