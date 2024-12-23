#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>

#define BUFFER_SIZE 512
#define SHM_NAME "/shared_mem"
#define SEM_PARENT_WRITE "/sem_parent_write"
#define SEM_CHILD_READ "/sem_child_read"
#define END_MARKER "END"

// Глобальные переменные для обработки сигналов
sem_t *sem_parent_write = NULL;
sem_t *sem_child_read = NULL;
char *shared_memory = NULL;
int shm_fd = -1;

// Очистка ресурсов
void cleanup_resources() {
    if (sem_parent_write != NULL) {
        sem_close(sem_parent_write);
    }
    if (sem_child_read != NULL) {
        sem_close(sem_child_read);
    }
    if (shared_memory != NULL) {
        munmap(shared_memory, BUFFER_SIZE);
    }
    if (shm_fd != -1) {
        close(shm_fd);
    }
}

// Обработчик сигналов
void signal_handler(int signum) {
    cleanup_resources();
    write(STDERR_FILENO, "Ресурсы дочернего процесса освобождены.\n", 42);
    exit(EXIT_FAILURE);
}

void write_error(const char *message) {
    if (message != NULL) {
        write(STDERR_FILENO, message, strlen(message));
    }
}

int process_command(const char *command) {
    char buffer[BUFFER_SIZE];
    strncpy(buffer, command, BUFFER_SIZE - 1);
    buffer[BUFFER_SIZE - 1] = '\0';

    char *token = strtok(buffer, " ");
    float result = 0.0;
    int is_first = 1;

    while (token != NULL) {
        float num = atof(token);
        if (is_first) {
            result = num;
            is_first = 0;
        } else {
            if (num == 0) {
                write_error("Ошибка: Деление на ноль.\n");
                return -1;
            }
            result /= num;
        }
        token = strtok(NULL, " ");
    }

    char output[BUFFER_SIZE];
    snprintf(output, BUFFER_SIZE, "Результат деления: %.6f\n", result);
    write(STDOUT_FILENO, output, strlen(output));

    return 0;
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        write_error("Ошибка: Не удалось открыть общую память.\n");
        return EXIT_FAILURE;
    }

    shared_memory = mmap(0, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        cleanup_resources();
        write_error("Ошибка: Не удалось отобразить общую память.\n");
        return EXIT_FAILURE;
    }

    sem_parent_write = sem_open(SEM_PARENT_WRITE, 0);
    sem_child_read = sem_open(SEM_CHILD_READ, 0);
    if (sem_parent_write == SEM_FAILED || sem_child_read == SEM_FAILED) {
        cleanup_resources();
        write_error("Ошибка: Не удалось открыть семафоры.\n");
        return EXIT_FAILURE;
    }

    while (1) {
        sem_wait(sem_parent_write);

        if (strcmp(shared_memory, END_MARKER) == 0) {
            break;
        }

        if (process_command(shared_memory) < 0) {
            break;
        }

        sem_post(sem_child_read);
    }

    cleanup_resources();
    return EXIT_SUCCESS;
}
