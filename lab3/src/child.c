#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>

#define BUFFER_SIZE 512
#define END_MARKER "END"

typedef enum Errors {
    E_SUCCESS = 0,
    E_INVALID_INPUT,
    E_DEVIDE_BY_ZERO
} ERRORS_EXIT_CODES;

void write_error(const char *error_string) {
    if (error_string == NULL) {
        write(STDERR_FILENO, "ERROR", 6);
    }
    write(STDERR_FILENO, error_string, strlen(error_string));
}

int process_command(const char *command) {
    char *token;
    float result = 0.0;
    int first = 1;

    // Копируем строку, чтобы не изменять оригинал
    char buffer[BUFFER_SIZE];
    int i = 0;
    while (command[i] != '\0' && i < BUFFER_SIZE - 1) {
        buffer[i] = command[i];
        i++;
    }
    buffer[i] = '\0'; // Нулевой символ для завершения строки

    token = strtok(buffer, " ");

    while (token != NULL) {
        float num = atof(token);

        if (first) {
            result = num;
            first = 0;
        } else {
            if (num == 0) {
                write_error("ERROR: Division by zero\n");
                return E_DEVIDE_BY_ZERO;
            }
            result /= num;
        }

        token = strtok(NULL, " ");
    }

    write(STDOUT_FILENO, "Division result is: ", 20);
    char result_str[BUFFER_SIZE];
    int length = snprintf(result_str, sizeof(result_str), "%f\n", result);
    write(STDOUT_FILENO, result_str, length);

    return E_SUCCESS;
}

int main() {
    // Открываем общую память
    int shm_fd = shm_open("/shared_mem", O_RDWR, 0666);
    if (shm_fd == -1) {
        write_error("ERROR: shm_open failed\n");
        exit(EXIT_FAILURE);
    }

    // Отображаем общую память
    char *shm_ptr = mmap(0, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        close(shm_fd); // Закрываем дескриптор памяти
        write_error("ERROR: mmap failed\n");
        exit(EXIT_FAILURE);
    }

    // Открываем семафоры для синхронизации
    sem_t *sem_parent_write = sem_open("/sem_parent_write", 0);
    sem_t *sem_child_read = sem_open("/sem_child_read", 0);
    if (sem_parent_write == SEM_FAILED || sem_child_read == SEM_FAILED) {
        munmap(shm_ptr, BUFFER_SIZE); // Освобождаем память
        close(shm_fd); // Закрываем дескриптор памяти
        write_error("ERROR: sem_open failed\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Ждем, пока родительский процесс напишет команду
        sem_wait(sem_parent_write);

        // Проверяем маркер завершения
        if (strcmp(shm_ptr, END_MARKER) == 0) {
            break; // Завершаем работу, если получен маркер
        }

        // Обрабатываем команду из общей памяти
        if (process_command(shm_ptr) == E_DEVIDE_BY_ZERO) {
            break;
        }

        // Сигнализируем родительскому процессу, что обработка завершена
        sem_post(sem_child_read);
    }

    // Закрываем семафоры
    sem_close(sem_parent_write);
    sem_close(sem_child_read);

    // Отключаем общую память
    munmap(shm_ptr, BUFFER_SIZE);
    close(shm_fd);

    return E_SUCCESS;
}
