// Created by Qwental on 22.12.2024.

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
#include <sys/mman.h>
#include <time.h>
#define SNPRINTF_BUF 256

typedef void *(*allocator_create_t)(void *const memory, const size_t size);

typedef void *(*allocator_alloc_t)(void *const allocator, const size_t size);

typedef void (*allocator_free_t)(void *const allocator, void *const memory);

typedef void (*allocator_destroy_t)(void *const allocator);

typedef struct {
    char first_name[52];
    char last_name[52];
    char group[15];
} Student;

void write_message(const char *message) {
    write(STDOUT_FILENO, message, strlen(message));
}

void write_error(const char *message) {
    write(STDERR_FILENO, message, strlen(message));
}

long long calculate_time_diff_ns(const struct timespec start, const struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        write_error("Usage: <program> <path_to_allocator_library>\n");
        return 52;
    }

    void *allocator_lib = dlopen(argv[1], RTLD_LAZY);
    if (allocator_lib == NULL) {
        write_error("Error loading library: ");
        write_error(dlerror());
        write_error("\n");
        return 1;
    }

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

    size_t pool_size = 4 * 1024 * 1024; // 4 MB
    void *memory_pool = mmap(NULL, pool_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory_pool == MAP_FAILED) {
        write_error("Memory allocation for pool failed (mmap)\n");
        dlclose(allocator_lib);
        return 1;
    }
    write_message("POOL SIZE is 4 MB = 4 * 1024 * 1024\n");

    void *allocator = allocator_create(memory_pool, pool_size);
    if (!allocator) {
        write_error("Allocator creation failed\n");
        munmap(memory_pool, pool_size);
        dlclose(allocator_lib);
        return 1;
    }

    struct timespec start, end;

    write_message("\n===============TEST1===============\n\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    Student *stud_array = (Student *) allocator_alloc(allocator, 5 * sizeof(Student));
    clock_gettime(CLOCK_MONOTONIC, &end);

    if (stud_array) {
        char buffer[SNPRINTF_BUF];
        snprintf(buffer, sizeof(buffer), "[[LOG]]Time to allocate: %lld ns\n", calculate_time_diff_ns(start, end));
        write_message(buffer);

        for (int i = 0; i < 5; i++) {
            snprintf(stud_array[i].first_name, sizeof(stud_array[i].first_name), "Vladimir%d", i + 1);
            snprintf(stud_array[i].last_name, sizeof(stud_array[i].last_name), "Bugrenkov%d", i + 1);
            snprintf(stud_array[i].group, sizeof(stud_array[i].group), "М8О-21%d-23", i);
        }

        write_message("Students before shuffle:\n");
        for (int i = 0; i < 5; i++) {
            snprintf(buffer, sizeof(buffer), "Name: %s, Last Name: %s, Group: %s\n",
                     stud_array[i].first_name, stud_array[i].last_name, stud_array[i].group);
            write_message(buffer);
        }

        Student temp = stud_array[0];
        stud_array[0] = stud_array[4];
        stud_array[4] = temp;

        write_message("Students after shuffle:\n");
        for (int i = 0; i < 5; i++) {
            snprintf(buffer, sizeof(buffer), "Name: %s, Last Name: %s, Group: %s\n",
                     stud_array[i].first_name, stud_array[i].last_name, stud_array[i].group);
            write_message(buffer);
        }

        clock_gettime(CLOCK_MONOTONIC, &start);
        allocator_free(allocator, stud_array);
        clock_gettime(CLOCK_MONOTONIC, &end);
        snprintf(buffer, sizeof(buffer), "[[LOG]]Time to free block: %lld ns\n", calculate_time_diff_ns(start, end));
        write_message(buffer);
    } else {
        write_error("Memory allocation for students failed\n");
    }


    write_message("\n===============TEST2===============\n\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    long double *long_double_array = (long double *) allocator_alloc(allocator, 20 * sizeof(long double));
    clock_gettime(CLOCK_MONOTONIC, &end);

    if (long_double_array) {
        char buffer[SNPRINTF_BUF];
        snprintf(buffer, sizeof(buffer), "[[LOG]]Time to allocate: %lld ns\n", calculate_time_diff_ns(start, end));
        write_message(buffer);

        for (int i = 0; i < 20; i++) {
            long_double_array[i] = i + 1;
        }
        write_message("array before mulp52:\n");
        for (int i = 0; i < 20; i++) {
            char buffer[SNPRINTF_BUF];
            snprintf(buffer, sizeof(buffer), "long_double_array[%d] = %Lf\n", i, long_double_array[i]);
            write_message(buffer);
        }
        for (int i = 0; i < 20; i++) {
            long_double_array[i] *= 52;
        }
        write_message("array after mulp52:\n");
        for (int i = 0; i < 20; i++) {
            char buffer[SNPRINTF_BUF];
            snprintf(buffer, sizeof(buffer), "long_double_array[%d] = %Lf\n", i, long_double_array[i]);
            write_message(buffer);
        }
        clock_gettime(CLOCK_MONOTONIC, &start);
        allocator_free(allocator, long_double_array);
        clock_gettime(CLOCK_MONOTONIC, &end);
        snprintf(buffer, sizeof(buffer), "[[LOG]]Time to free block: %lld ns\n", calculate_time_diff_ns(start, end));
        write_message(buffer);
    } else {
        write_error("Memory allocation for long double array failed\n");
    }
    write_message("\n===============TEST3===============\n\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    long double *large_array = (long double *) allocator_alloc(allocator, 5001 * sizeof(long double));
    clock_gettime(CLOCK_MONOTONIC, &end);


    if (large_array) {
        char buffer[SNPRINTF_BUF];
        snprintf(buffer, sizeof(buffer), "[[LOG]]Time to allocate: %lld ns\n", calculate_time_diff_ns(start, end));
        write_message(buffer);
        large_array[5000] = 52;
        snprintf(buffer, sizeof(buffer), "large_array[5000] = %Lf\n", large_array[5000]);
        write_message(buffer);
        write_message("Large array (5001 long double) allocated and freed successfully\n");

        clock_gettime(CLOCK_MONOTONIC, &start);
        allocator_free(allocator, large_array);
        clock_gettime(CLOCK_MONOTONIC, &end);
        snprintf(buffer, sizeof(buffer), "[[LOG]]Time to free block: %lld ns\n", calculate_time_diff_ns(start, end));
        write_message(buffer);
    } else {
        write_error("Memory allocation for large array failed\n");
    }

    write_message("\n===============TEST4===============\n\n");
    clock_gettime(CLOCK_MONOTONIC, &start);

    void *block1 = allocator_alloc(allocator, 3 * 1024 * 1024);

    clock_gettime(CLOCK_MONOTONIC, &end);

    if (block1) {
        char buffer[SNPRINTF_BUF];
        snprintf(buffer, sizeof(buffer), "[[LOG]]Time to allocate: %lld ns\n", calculate_time_diff_ns(start, end));
        write_message(buffer);

        write_message("Memory allocated (3 * 1024 * 1024 bytes)\n");
        clock_gettime(CLOCK_MONOTONIC, &start);
        allocator_free(allocator, block1);
        clock_gettime(CLOCK_MONOTONIC, &end);
        snprintf(buffer, sizeof(buffer), "[[LOG]]Time to free block: %lld ns\n", calculate_time_diff_ns(start, end));
        write_message(buffer);
        write_message("Memory freed (3 * 1024 * 1024 bytes)\n");
    } else {
        write_error("Memory allocation failed\n");
    }

    allocator_destroy(allocator);
    write_message("Allocator destroyed\n");
    munmap(memory_pool, pool_size);
    dlclose(allocator_lib);
    return 0;
}
