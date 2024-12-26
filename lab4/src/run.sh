#!/bin/bash

# Устанавливаем флаг выхода из скрипта при ошибке
set -e

# === Компиляция библиотеки mckusick_carels ===
echo "Компиляция libmckusick_carels.so..."
gcc -shared -fPIC -o libmckusick_carels.so mckusick_carels.c
echo "libmckusick_carels.so успешно скомпилирована."

echo ""

# === Компиляция библиотеки block_2n ===
echo "Компиляция libblock_2n.so..."
gcc -shared -fPIC -o libblock_2n.so block_2n.c
echo "libblock_2n.so успешно скомпилирована."

echo ""

# === Компиляция основного файла main.c ===
echo "Компиляция main.c..."
gcc -o main main.c -ldl
echo "main успешно скомпилирован."

echo ""

# === Тестирование с libblock_2n.so ===
echo "Тестирование с libblock_2n.so..."
./main ./libblock_2n.so
echo "Тестирование с libblock_2n.so завершено."

echo ""

# === Тестирование с libmckusick_carels.so ===
echo "Тестирование с libmckusick_carels.so..."
./main ./libmckusick_carels.so
echo "Тестирование с libmckusick_carels.so завершено."

echo ""

# === Завершение работы скрипта ===
echo "Все библиотеки успешно скомпилированы и протестированы."
