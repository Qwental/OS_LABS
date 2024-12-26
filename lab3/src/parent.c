#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <signal.h>

#define BUFFER_SIZE 512
#define SHM_NAME "/shared_mem"
#define SEM_PARENT_WRITE "/sem_parent_write"
#define SEM_CHILD_READ "/sem_child_read"
#define END_MARKER "END"

// Глобальные переменные для обработки сигналов Конечно лучше через sigaction, но это оверкилл уже
sem_t *sem_parent_write = NULL;
sem_t *sem_child_read = NULL;
char *shared_memory = NULL;
int shm_fd = -1;

// Очистка ресурсов
void cleanup_resources() {
    if (sem_parent_write != NULL) {
        sem_close(sem_parent_write);
        sem_unlink(SEM_PARENT_WRITE);
        sem_parent_write = NULL;
    }
    if (sem_child_read != NULL) {
        sem_close(sem_child_read);
        sem_unlink(SEM_CHILD_READ);
        sem_child_read = NULL;
    }
    if (shared_memory != NULL) {
        munmap(shared_memory, BUFFER_SIZE);
        shared_memory = NULL;
    }
    if (shm_fd != -1) {
        close(shm_fd);
        shm_unlink(SHM_NAME);
        shm_fd = -1;
    }
}

// Обработчик сигналов
void signal_handler(int signum) {
    cleanup_resources();
    write(STDERR_FILENO, "Ресурсы освобождены, программа завершена.\n", 42);
    exit(EXIT_FAILURE);
}

void write_error(const char *message) {
    if (message != NULL) {
        write(STDERR_FILENO, message, strlen(message));
    }
}

int main(int argc, char *argv[]) {
    // Установка обработчиков сигналов
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (argc != 2) {
        write_error("Ошибка: Укажите имя файла в качестве аргумента.\n");
        return EXIT_FAILURE;
    }

    FILE *input_file = fopen(argv[1], "r");
    if (input_file == NULL) {
        write_error("Ошибка: Не удалось открыть файл.\n");
        cleanup_resources();
        return EXIT_FAILURE;
    }

    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        fclose(input_file);
        cleanup_resources();
        write_error("Ошибка: Не удалось создать объект общей памяти.\n");
        return EXIT_FAILURE;
    }

    if (ftruncate(shm_fd, BUFFER_SIZE) == -1) {
        fclose(input_file);
        cleanup_resources();
        write_error("Ошибка: Не удалось установить размер общей памяти.\n");
        return EXIT_FAILURE;
    }

    shared_memory = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        fclose(input_file);
        cleanup_resources();
        write_error("Ошибка: Не удалось отобразить общую память.\n");
        return EXIT_FAILURE;
    }
    // создаем семафоры
    sem_parent_write = sem_open(SEM_PARENT_WRITE, O_CREAT, 0666, 0);
    sem_child_read = sem_open(SEM_CHILD_READ, O_CREAT, 0666, 0);
    if (sem_parent_write == SEM_FAILED || sem_child_read == SEM_FAILED) {
        fclose(input_file);
        cleanup_resources();
        write_error("Ошибка: Не удалось создать семафоры.\n");
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid < 0) {
        fclose(input_file);
        cleanup_resources();

        write_error("Ошибка: Не удалось создать дочерний процесс.\n");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        execl("./child", "./child", NULL);

        fclose(input_file);
        cleanup_resources();
        write_error("Ошибка: Не удалось выполнить дочерний процесс.\n");
        exit(EXIT_FAILURE);
    } else {
        char buffer[BUFFER_SIZE];
        while (fgets(buffer, sizeof(buffer), input_file) != NULL) {
            strncpy(shared_memory, buffer, BUFFER_SIZE - 1);
            shared_memory[BUFFER_SIZE - 1] = '\0';
            sem_post(sem_parent_write); // увед дочер процесс +
            sem_wait(sem_child_read); // ожидаем

            // Проверяем, не произошла ли ошибка в дочернем процессе
            if (strcmp(shared_memory, "ERROR") == 0) {
                write_error("Ошибка в дочернем процессе. Завершаем работу.\n");
                cleanup_resources();
                fclose(input_file);
                exit(EXIT_FAILURE);
            }
        }
        // записываем end в SH и ждемс
        strncpy(shared_memory, END_MARKER, BUFFER_SIZE - 1);
        shared_memory[BUFFER_SIZE - 1] = '\0';
        sem_post(sem_parent_write);
        wait(NULL);
    }

    cleanup_resources();
    fclose(input_file);

    return EXIT_SUCCESS;
}
