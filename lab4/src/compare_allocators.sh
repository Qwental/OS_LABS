#!/bin/bash

# Убедимся, что hyperfine установлен
if ! command -v hyperfine &> /dev/null
then
    echo "Ошибка: hyperfine не установлен. Установите его с помощью 'sudo apt install hyperfine'."
    exit 1
fi

# Компиляция библиотек
echo "Компиляция libmckusick_carels.so..."
gcc -shared -fPIC -o libmckusick_carels.so mckusick_carels.c
echo "libmckusick_carels.so успешно скомпилирована."

echo "Компиляция libblock_2n.so..."
gcc -shared -fPIC -o libblock_2n.so block_2n.c
echo "libblock_2n.so успешно скомпилирована."

# Компиляция main.c
echo "Компиляция main.c..."
gcc -o main main.c -ldl
echo "main успешно скомпилирован."

echo ""

# Использование hyperfine для замера времени
echo "=== Сравнение времени выполнения аллокаторов ==="
hyperfine \
    --warmup 10 \
    './main ./libblock_2n.so' \
    './main ./libmckusick_carels.so'

echo "Сравнение завершено."
