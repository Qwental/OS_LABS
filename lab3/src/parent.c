#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/stat.h>

#define BUFFER_SIZE 512
#define END_MARKER "END"

typedef enum Errors {
    E_SUCCESS = 0,
    E_INVALID_INPUT,
    E_CANNOT_OPEN_FILE,
    E_PIPE_FAILED,
    E_SEMAPHORE_FAILED
} ERRORS_EXIT_CODES;

void write_error(const char *error_string) {
    if (error_string == NULL) {
        write(STDERR_FILENO, "ERROR", 6);
    }
    write(STDERR_FILENO, error_string, strlen(error_string));
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        write_error("ERROR: Missing filename argument\n");
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        write_error("ERROR: Cannot open file\n");
        exit(EXIT_FAILURE);
    }

    // Создаем общую память
    int shm_fd = shm_open("/shared_mem", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        fclose(file);
        write_error("ERROR: shm_open failed\n");
        exit(EXIT_FAILURE);
    }

    // Устанавливаем размер общей памяти
    if (ftruncate(shm_fd, BUFFER_SIZE) == -1) {
        close(shm_fd); // Закрываем дескриптор памяти
        fclose(file);
        write_error("ERROR: ftruncate failed\n");
        exit(EXIT_FAILURE);
    }

    // Отображаем общую память
    char *shm_ptr = mmap(0, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        close(shm_fd); // Закрываем дескриптор памяти
        fclose(file); // Освобождаем файл
        write_error("ERROR: mmap failed\n");
        exit(EXIT_FAILURE);
    }

    // Создаем семафоры для синхронизации
    sem_t *sem_parent_write = sem_open("/sem_parent_write", O_CREAT, 0666, 0);
    sem_t *sem_child_read = sem_open("/sem_child_read", O_CREAT, 0666, 0);
    if (sem_parent_write == SEM_FAILED || sem_child_read == SEM_FAILED) {
        munmap(shm_ptr, BUFFER_SIZE); // Освобождаем память
        close(shm_fd); // Закрываем дескриптор памяти
        fclose(file); // Освобождаем файл
        write_error("ERROR: sem_open failed\n");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0) {
        sem_close(sem_parent_write);
        sem_close(sem_child_read);
        sem_unlink("/sem_parent_write");
        sem_unlink("/sem_child_read");

        munmap(shm_ptr, BUFFER_SIZE);
        close(shm_fd);
        shm_unlink("/shared_mem");
        fclose(file);
        write_error("ERROR: fork failed\n");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Дочерний процесс
        close(shm_fd);
        execl("./child", "", NULL);
        write_error("ERROR: execl failed\n");
        exit(EXIT_FAILURE);
    } else {
        // Родительский процесс
        char file_buffer[BUFFER_SIZE];
        while (fgets(file_buffer, sizeof(file_buffer), file) != NULL) {
            // Записываем команду в общую память
            int i = 0;
            while (file_buffer[i] != '\0' && i < BUFFER_SIZE - 1) {
                shm_ptr[i] = file_buffer[i];
                i++;
            }
            shm_ptr[i] = '\0'; // Добавляем нулевой символ в конец строки

            // Сигнализируем дочернему процессу, что данные готовы для обработки
            sem_post(sem_parent_write);

            // Ждем, пока дочерний процесс завершит обработку
            sem_wait(sem_child_read);
        }

        // Отправляем специальный маркер для завершения работы дочернего процесса
        int i = 0;
        while (END_MARKER[i] != '\0' && i < BUFFER_SIZE - 1) {
            shm_ptr[i] = END_MARKER[i];
            i++;
        }
        shm_ptr[i] = '\0';
        sem_post(sem_parent_write); // Дочерний процесс получит маркер завершения

        // Закрываем семафоры
        sem_close(sem_parent_write);
        sem_close(sem_child_read);

        // Закрываем файл
        fclose(file);

        // Ожидаем завершения дочернего процесса
        wait(NULL);
    }

    sem_close(sem_parent_write);
    sem_close(sem_child_read);
    sem_unlink("/sem_parent_write");
    sem_unlink("/sem_child_read");

    munmap(shm_ptr, BUFFER_SIZE);
    close(shm_fd);
    shm_unlink("/shared_mem");
    fclose(file);

    return E_SUCCESS;
}
