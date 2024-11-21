#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <limits.h>
#include <pthread.h>
#include <float.h>
// clang -pedantic -std=c2x task2.c -o task2 
//  ./main 1 911111 12
#define MAX_THREADS 33

void write_error(const char *error_string)
{
    if (error_string == NULL)
    {
        write(STDERR_FILENO, "ERROR\n", 7);
    }
    write(STDERR_FILENO, error_string, strlen(error_string));
}

// будем использовать структуру, тк pthread_create принимает только один аргумент
typedef struct
{
    long *inside_points;
    pthread_mutex_t *mutex;
    long radius;
    long numPoints_per_Thread;

} THREADS_POINTS;

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
ERRORS_EXIT_CODES string_to_long_int(const char *str_number, long *int_result_number, int base)
{
    if (str_number == NULL || int_result_number == NULL)
        return E_INVALID_INPUT;
    char *endptr;
    *int_result_number = strtol(str_number, &endptr, base);
    if (*int_result_number == LONG_MAX || *int_result_number == LONG_MIN)
        return E_TYPE_OVERFLOW;
    else if (*endptr != '\0')
        return E_INVALID_INPUT;
    return E_SUCCESS;
}
/* Проверка на переполнение типа double */
int is_double_overflow(float value)
{

    if (value > DBL_MAX || value < -DBL_MAX || value == HUGE_VALF || value == -HUGE_VALF)
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
ERRORS_EXIT_CODES string_to_double(const char *str, double *num)
{
    if (str == NULL || num == NULL)
        return E_DEREFENCE_NULL_POINTER;

    char *endptr;
    double value = strtod(str, &endptr);

    if (*endptr != '\0')
        return E_INVALID_INPUT;

    if (is_double_overflow(value))
        return E_DOUBLE_OVERFLOW;

    *num = value;
    return E_SUCCESS;
}

// Функция для расчёта площади методом Монте-Карло
void *monteCarloCircleArea(void *arg)
{
    double x, y, radius;
    long pointsInCircle = 0;
    THREADS_POINTS *thread_points = (THREADS_POINTS *)arg;
    radius = thread_points->radius;
    long numPoints = thread_points->numPoints_per_Thread;

    /* rand() - cringe, очень медленно считает, поэтому юзаем rand_r*/
    time_t seed = time(NULL);
    for (long i = 0; i < numPoints; i++)
    {
        // Генерируем случайные x и y в диапазоне [-R, R]
        x = ((double)rand_r(&seed) / RAND_MAX) * 2 * radius - radius;
        y = ((double)rand_r(&seed) / RAND_MAX) * 2 * radius - radius;

        // Проверяем, попадает ли точка в окружность
        if (x * x + y * y <= radius * radius)
        {
            pointsInCircle++;
        }
    }
    pthread_mutex_lock(thread_points->mutex);
    *(thread_points->inside_points) += pointsInCircle;
    pthread_mutex_unlock(thread_points->mutex);

    // // Площадь окружности
    // double squareArea = 4 * radius * radius;
    // double circleArea = ((double)pointsInCircle / numPoints) * squareArea;

    return NULL;
}

void write_result(const char *result)
{
    if (result == NULL)
        return;

    if (write(STDOUT_FILENO, result, strlen(result)) == -1)
        exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{

    if (argc != 4 || argv == NULL || argv[1] == NULL || argv[2] == NULL || argv[3] == NULL)
    {
        write_error("ERROR: INVALID INPUT, must be 1)radius 2)number of points 3)number of threads\n");
        return E_INVALID_INPUT;
    }

    double radius;
    long numPoints;
    long numThreads;
    /* Корректно обрабатываем числа*/
    ERRORS_EXIT_CODES error = string_to_double(argv[1], &radius);
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
    else if (error != E_SUCCESS)
    {
        write_error("ERROR\n");
        return error;
    }

    error = string_to_long_int(argv[2], &numPoints, 10);
    if (error == E_TYPE_OVERFLOW)
    {
        write_error("ERROR_INT_OVERFLOW\n");
        return error;
    }
    else if (error == E_INVALID_INPUT)
    {
        write_error("INVALID INPUT IN FILE\n");
        return error;
    }
    else if (error != E_SUCCESS)
    {
        write_error("ERROR\n");
        return error;
    }

    error = string_to_long_int(argv[3], &numThreads, 10);
    if (error == E_TYPE_OVERFLOW)
    {
        write_error("ERROR_INT_OVERFLOW\n");
        return error;
    }
    else if (error == E_INVALID_INPUT)
    {
        write_error("INVALID INPUT IN FILE\n");
        return error;
    }
    else if (error != E_SUCCESS)
    {
        write_error("ERROR\n");
        return error;
    }
    if (radius < 0 || numPoints <= 0 || numThreads > MAX_THREADS)
    {
        write_error("ERROR: INVALID_INPUT\n");
        return E_INVALID_INPUT;
    }

    pthread_t *threads = (pthread_t *)malloc(numThreads * sizeof(pthread_t));
    if (threads == NULL)
    {
        write_error("ERROR: MEMORY ALLOCATION\n");
        return E_MEMORY_ALLOCATION;
    }

    THREADS_POINTS *thread_points = (THREADS_POINTS *)malloc(numThreads * sizeof(THREADS_POINTS));
    if (thread_points == NULL)
    {
        free(threads);
        write_error("ERROR: MEMORY ALLOCATION\n");
        return E_MEMORY_ALLOCATION;
    }

    long numPoints_per_Thread = numPoints / numThreads;
    long pointsInCircle = 0;

    pthread_mutex_t mutex;
    if (pthread_mutex_init(&mutex, NULL) != 0)
    {
        free(threads);
        free(thread_points);
        write_error("ERROR: MUTEX INIT\n");
        return E_MEMORY_ALLOCATION;
    }

    for (long i = 0; i < numThreads; i++)
    {
        thread_points[i].radius = radius;
        thread_points[i].numPoints_per_Thread = numPoints_per_Thread;
        thread_points[i].inside_points = &pointsInCircle;
        thread_points[i].mutex = &mutex;
    }

    for (long i = 0; i < numThreads; i++)
    {
        pthread_create(threads + i, NULL, monteCarloCircleArea, thread_points + i);
    }

    for (long i = 0; i < numThreads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);

    double circle_area = (double)pointsInCircle / numPoints * 4 * radius * radius;
    char result[BUFSIZ];
    if (sprintf(result, "Square = %lf\n", circle_area) < 0)
    {
        free(threads);
        free(thread_points);
        write_error("ERROR: BUFFER OVERFLOW\n");
        return E_BUFFER_OVERFLOW;
    }
    write_result(result);
    free(threads);
    free(thread_points);

    

    return E_SUCCESS;
}
