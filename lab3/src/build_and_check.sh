#!/bin/bash
#  chmod +x build_and_check.sh
#gcc -o child child.c -lrt -lpthread
#gcc -o parent parent.c -lrt -lpthread

# ./build_and_check.sh
# ls /dev/shm
# Имена файлов и программ
PARENT_SRC="parent.c"
CHILD_SRC="child.c"
PARENT_BIN="parent"
CHILD_BIN="child"
SHM_OBJECT="/shared_mem"
SEM_PARENT="/sem_parent_write"
SEM_CHILD="/sem_child_read"

# Компиляция
echo "===Компиляция parent.c==="
gcc -o $PARENT_BIN $PARENT_SRC -lrt -lpthread
if [ $? -ne 0 ]; then
    echo "Ошибка компиляции parent.c"
    exit 1
fi

echo "===Компиляция child.c==="
gcc -o $CHILD_BIN $CHILD_SRC -lrt -lpthread
if [ $? -ne 0 ]; then
    echo "Ошибка компиляции child.c"
    exit 1
fi

# Проверка наличия ресурсов до запуска
echo "===Проверка наличия объектов в /dev/shm перед запуском==="
ls /dev/shm | grep -E "($SHM_OBJECT|$SEM_PARENT|$SEM_CHILD)"
if [ $? -eq 0 ]; then
    echo "Обнаружены остаточные ресурсы в /dev/shm перед запуском. Удалите их вручную!"
    exit 1
fi

# Запуск программы
echo "===Запуск программы==="
./$PARENT_BIN test.txt

# Проверка наличия ресурсов после завершения
echo "===Проверка наличия объектов в /dev/shm после завершения==="
ls /dev/shm | grep -E "($SHM_OBJECT|$SEM_PARENT|$SEM_CHILD)"
if [ $? -eq 0 ]; then
    echo "Ошибка: ресурсы не были освобождены!"
    echo "Оставшиеся объекты:"
    ls /dev/shm | grep -E "($SHM_OBJECT|$SEM_PARENT|$SEM_CHILD)"
    exit 1
else
    echo "Все ресурсы были успешно освобождены."
fi

# Очистка
echo "===Очистка бинарных файлов==="
rm -f $PARENT_BIN $CHILD_BIN

echo "Скрипт завершён успешно."
