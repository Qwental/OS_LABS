#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include "string.h"
#include <limits.h>
#include <float.h>
#include <math.h>
#include <float.h>

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

/* Перевод строки в int */
ERRORS_EXIT_CODES string_to_int(const char *str_number, int *int_result_number, int base)
{
    if (str_number == NULL || int_result_number == NULL)
        return E_INVALID_INPUT;
    char *endptr;
    *int_result_number = strtol(str_number, &endptr, base);
    if (*int_result_number == INT_MAX || *int_result_number == INT_MIN)
        return E_TYPE_OVERFLOW;
    else if (*endptr != '\0')
        return E_INVALID_INPUT;
    return E_SUCCESS;
}

int is_float_overflow(float value)
{

    // Проверка на превышение предельных значений для типа float
    if (value > FLT_MAX || value < -FLT_MAX || value == HUGE_VALF || value == -HUGE_VALF)
    {
        return 1;
    }
    if (isinf(value))
    {
        return 1;
    }

    return 0;
}

/* Перевод строки в double */
ERRORS_EXIT_CODES string_to_float(const char *str, float *num)
{
    if (str == NULL || num == NULL)
        return E_DEREFENCE_NULL_POINTER;

    char *endptr;
    float value = strtof(str, &endptr);

    if (*endptr != '\0')
        return E_INVALID_INPUT;

    if (is_float_overflow(value))
        return E_DOUBLE_OVERFLOW;

    *num = value;
    return E_SUCCESS;
}

void write_error(const char *error_string)
{
    if (error_string == NULL)
    {
        write(STDOUT_FILENO, "ERROR", 6);
    }
    write(STDOUT_FILENO, error_string, strlen(error_string));
}

void write_devision_result(float res)
{
    char string[BUFFER_SIZE];
    int len_str = snprintf(string, sizeof(string), "Devision result is: %f\n", res);
    write(STDOUT_FILENO, string, len_str);
}

int main()
{
    char file_buffer[BUFFER_SIZE];

    while (fgets(file_buffer, sizeof(file_buffer), stdin) != NULL)
    {

        float devision_res = 0;
        int flag = 1;

        file_buffer[strcspn(file_buffer, "\n")] = '\0'; // Получаем строку
        char *token = strtok(file_buffer, " ");         // Сплитим по пробелам

        while (token != NULL)
        {
            float res = 0;
            ERRORS_EXIT_CODES error = string_to_float(token, &res);

            if (error == E_DOUBLE_OVERFLOW)
            {
                write_error("ERROR_DOUBLE_OVERFLOW\n");
                return error;
            }
            else if (error == E_INVALID_INPUT)
            {
                write_error("INVALID INPUT IN FILE\n");
                return error;
            }
            else if (error == E_SUCCESS)
            {
                if (res == 0.0 || fabsf(res) < FLT_EPSILON)
                {
                    write_error("ERROR DEVISION BY ZERO\n");
                    return E_INVALID_INPUT;
                }
                if (flag)
                {
                    flag = 0;
                    devision_res = res;
                }
                else
                {
                    devision_res /= res;
                }
            }
            else
            {
                write_error("ERROR\n");
                return error;
            }

            token = strtok(NULL, " ");
        }

        write_devision_result(devision_res);
    }

    return E_SUCCESS;
}