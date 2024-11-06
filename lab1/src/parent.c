#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

#define BUFFER_SIZE 512

/* ERRORS_EXIT_CODES или enum Errors - КОДЫ ВОЗВРАТА ДЛЯ ФУНКЦИЙ */
typedef enum Errors
{
    E_SUCCESS = 0,            /* Успешное завершение */
    E_INVALID_INPUT,          /* Ошибка: Некорректный ввод */
    E_NOT_ENOUGH_PARAMS,      /* Ошибка: Отсутвует аргумент */
    E_INVALID_ARG,            /* Ошибка: Неправильный аргумент */
    E_TYPE_OVERFLOW,          /* Ошибка: Недостаточно памяти для записи значения некоторого типа */
    E_MEMORY_ALLOCATION,      /* Ошибка: Выделении памяти */
    E_CANNOT_OPEN_FILE,       /* Ошибка: ОТКРЫТИЯ ФАЙЛА */
    E_INVALID_EPSILON,        /* Ошибка: Невалидного числа эпсилон*/
    E_DEREFENCE_NULL_POINTER, /* Ошибка: Попытка разыменовать Dereference NULL */
    E_INT_OVERFLOW,           /* Ошибка: Переполнение типа INT */
    E_LONG_OVERFLOW,          /* Ошибка: Переполнение типа LONG INT */
    E_FLOAT_OVERFLOW,         /* Ошибка: Переполнение типа FLOAT */
    E_DOUBLE_OVERFLOW,        /* Ошибка: Переполнение типа DOUBLE */
    E_LONG_DOUBLE_OVERFLOW,   /* Ошибка: Переполнение типа DOUBLE */
    E_INVALID_FLAG_ARG,       /* Ошибка: Неккорекный ввод аргументов (флагов), они должны начинаться с символов '-' или '/' */
    E_SAME_FILE_NAMES,        /* Ошибка: Одинаковые имена файлов */
    E_BUFFER_OVERFLOW,        /* Ошибка: Переполнение буффера */
    E_SAME_FILES,             /* Ошибка: Ввод одинковых файлов */
    E_FALSE,                  /* BOOL_FALSE */

} ERRORS_EXIT_CODES;

void write_error(const char *error_string)
{
    if (error_string == NULL)
    {
        write(STDERR_FILENO, "ERROR", 6);
    }
    write(STDERR_FILENO, error_string, strlen(error_string));
}

int main(int args, char *argv[])
{

    if (args != 2)
    {
        write_error("ERROR: User: where is a filename????\n");
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL)
    {
        write_error("ERROR: cannot open file\n");
        exit(EXIT_FAILURE);
    }

    int fd[2];
    /*
    fd[0] read
    fd[1] write
    */
    if (pipe(fd) == -1)
    {
        write_error("ERROR: Pipe failed\n");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        write_error("ERROR: Fork failed\n");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        // CHILD
        close(fd[1]);

        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);

        execl("./child", "", NULL);

        write_error("ERROR: execl failed, use gcc child.c with flag -o child\n");
        exit(EXIT_FAILURE);
    }

    else
    {
        // PARENT
        close(fd[0]);

        char file_buffer[BUFFER_SIZE];
        while (fgets(file_buffer, sizeof(file_buffer), file) != NULL)
        {
            write(fd[1], file_buffer, strlen(file_buffer));
        }
        close(fd[1]);

        fclose(file);
        wait(NULL);
    }

    return E_SUCCESS;
}
