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

// Очистка ресурсов
void cleanup_resources(sem_t *sem_parent_write, sem_t *sem_child_read, char *shared_memory, int shm_fd) {
    if (sem_parent_write != NULL) {
        sem_close(sem_parent_write);
        sem_unlink(SEM_PARENT_WRITE);
    }
    if (sem_child_read != NULL) {
        sem_close(sem_child_read);
        sem_unlink(SEM_CHILD_READ);
    }
    if (shared_memory != NULL) {
        munmap(shared_memory, BUFFER_SIZE);
    }
    if (shm_fd != -1) {
        close(shm_fd);
        shm_unlink(SHM_NAME);
    }
}

// Обработчик сигналов
void signal_handler(int signum) {
    write(STDERR_FILENO, "Программа прервана.\n", 21);
    exit(EXIT_FAILURE);
}

void write_error(const char *message) {
    if (message != NULL) {
        write(STDERR_FILENO, message, strlen(message));
    }
}

int main(int argc, char *argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (argc != 2) {
        write_error("Ошибка: Укажите имя файла в качестве аргумента.\n");
        return EXIT_FAILURE;
    }

    FILE *input_file = fopen(argv[1], "r");
    if (input_file == NULL) {
        write_error("Ошибка: Не удалось открыть файл.\n");
        return EXIT_FAILURE;
    }

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        fclose(input_file);
        write_error("Ошибка: Не удалось создать объект общей памяти.\n");
        return EXIT_FAILURE;
    }

    if (ftruncate(shm_fd, BUFFER_SIZE) == -1) {
        fclose(input_file);
        cleanup_resources(NULL, NULL, NULL, shm_fd);
        write_error("Ошибка: Не удалось установить размер общей памяти.\n");
        return EXIT_FAILURE;
    }

    char *shared_memory = mmap(0, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        fclose(input_file);
        cleanup_resources(NULL, NULL, NULL, shm_fd);
        write_error("Ошибка: Не удалось отобразить общую память.\n");
        return EXIT_FAILURE;
    }

    sem_t *sem_parent_write = sem_open(SEM_PARENT_WRITE, O_CREAT, 0666, 0);
    sem_t *sem_child_read = sem_open(SEM_CHILD_READ, O_CREAT, 0666, 0);
    if (sem_parent_write == SEM_FAILED || sem_child_read == SEM_FAILED) {
        fclose(input_file);
        cleanup_resources(sem_parent_write, sem_child_read, shared_memory, shm_fd);
        write_error("Ошибка: Не удалось создать семафоры.\n");
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid < 0) {
        fclose(input_file);
        cleanup_resources(sem_parent_write, sem_child_read, shared_memory, shm_fd);
        write_error("Ошибка: Не удалось создать дочерний процесс.\n");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        execl("./child", "./child", NULL);
        write_error("Ошибка: Не удалось выполнить дочерний процесс.\n");
        exit(EXIT_FAILURE);
    } else {
        char buffer[BUFFER_SIZE];
        while (fgets(buffer, sizeof(buffer), input_file) != NULL) {
            strncpy(shared_memory, buffer, BUFFER_SIZE - 1);
            shared_memory[BUFFER_SIZE - 1] = '\0';
            sem_post(sem_parent_write);
            sem_wait(sem_child_read);
        }

        strncpy(shared_memory, END_MARKER, BUFFER_SIZE - 1);
        shared_memory[BUFFER_SIZE - 1] = '\0';
        sem_post(sem_parent_write);
        wait(NULL);
    }

    cleanup_resources(sem_parent_write, sem_child_read, shared_memory, shm_fd);
    fclose(input_file);

    return EXIT_SUCCESS;
}
