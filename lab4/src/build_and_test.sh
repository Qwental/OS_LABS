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

# === Проверка Valgrind для libblock_2n.so ===
echo "Запуск Valgrind для libblock_2n.so..."
valgrind --leak-check=full --track-origins=yes ./main ./libblock_2n.so
echo "Valgrind завершен для libblock_2n.so."

echo ""

# === Проверка Valgrind для libmckusick_carels.so ===
echo "Запуск Valgrind для libmckusick_carels.so..."
valgrind --leak-check=full --track-origins=yes ./main ./libmckusick_carels.so
echo "Valgrind завершен для libmckusick_carels.so."

echo ""

# === Запуск strace для libblock_2n.so ===
echo "Запуск strace для libblock_2n.so..."
strace -o strace_block_2n.log ./main ./libblock_2n.so
echo "strace завершен для libblock_2n.so. Логи сохранены в strace_block_2n.log."

echo ""

# === Запуск strace для libmckusick_carels.so ===
echo "Запуск strace для libmckusick_carels.so..."
strace -o strace_mckusick_carels.log ./main ./libmckusick_carels.so
echo "strace завершен для libmckusick_carels.so. Логи сохранены в strace_mckusick_carels.log."

echo ""

# === Завершение работы скрипта ===
echo "Все библиотеки успешно скомпилированы, протестированы, и проверены Valgrind и strace."
