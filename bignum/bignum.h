#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <gmp.h>

enum BIGNUM_ERRORS {
  ERR_REALLOC_FAILED = 1,
  ERR_MALLOC_FAILED,
  ERR_NULLPTR,
  ERR_INVALID_SIZE,
  ERR_INVALID_STRING,
  ERR_DIVISION_BY_ZERO,
  
};

/**
 * @struct Описание большого числа
 * 
 */
typedef struct bignum_t
{
  uint32_t *val;      /* Разряды числа */
  size_t    length;   /* Реальная разрядность числа */
  size_t    capacity; /* Максимальная разрядность числа */
  bool      negative; /* Знак числа */

} bignum_t;

/**
 * @brief Инициализирует поля bignum_t начальными значениями.
 * 
 * @param val Указатель на число типа bignum_t.
 * 
 * @note Память под цифры не выделяется. Первое выделение произойдёт
 * при первой записи (from_*, add, mul и т.д.).
 */
void bignum_init(bignum_t *val);
void bignum_init(bignum_t *val);
/**
 * @brief Освобождает память занятую числом bignum_t
 * 
 * @param val Число типа bignum_t
 */
void bignum_delete(bignum_t *val);
/**
 * @brief Копирует содержимое src в dest,
 * включая число, его разрядность, знак и емкость
 * 
 * @param dest Число-приемник типа bignum_t
 * @param src Число-источник типа bignum_t
 * @return 0 в случае успеха, отрицательное значение 
 * из enum BIGNUM_ERRORS в случае ошибки
 */
int bignum_copy(bignum_t *dest, const bignum_t *src);
/**
 * @brief Печатает в консоль число bignum_t в hex
 * 
 * @param val Число типа bignum_t
 */
void bignum_print(const bignum_t *val);
/**
 * @brief Печатает в консоль число bignum_t в dec
 * 
 * @param val Число типа bignum_t
 */
void bignum_print_dec(const bignum_t *val);

/* --------------------Функции импорта-------------------- */

/**
 * @brief Инициализирует число bignum_t из числа, разрядностью 1 байт
 * 
 * @param dest Число-приемник bignum_t
 * @param src Число-источник в виде массива uint8_t
 * @param size Размер массива src (число байт)
 * @param neg Знак числа src
 * @return 0 в случае успеха, отрицательное значение 
 * из enum BIGNUM_ERRORS в случае ошибки
 */
int bignum_from_ui8_array(bignum_t *dest, const uint8_t *src, size_t size, bool neg);
/**
 * @brief Инициализирует число bignum_t из числа, разрядностью 4 байта
 * 
 * @param dest Число-приемник bignum_t
 * @param src Число-источник в виде массива uint32_t
 * @param size Размер массива src
 * @param neg Знак числа src
 * @return 0 в случае успеха, отрицательное значение 
 * из enum BIGNUM_ERRORS в случае ошибки 
 */
int bignum_from_ui32_array(bignum_t *dest, const uint32_t *src, size_t size, bool neg);
/**
 * @brief Инициализирует число bignum_t из строки, содержащей hex-символы
 * 
 * @param dest Число-приемник bignum_t
 * @param hex Строка hex-символов
 * @param neg Знак числа src
 * @return 0 в случае успеха, отрицательное значение 
 * из enum BIGNUM_ERRORS в случае ошибки 
 */
int bignum_from_hex_string(bignum_t *dest, const char *hex, bool neg);
/**
 * @brief Инициализирует число bignum_t из числа типа mpz_t
 * 
 * @param dest Число-приемник bignum_t
 * @param src Число-источник mpz_t
 * @return 0 в случае успеха, отрицательное значение 
 * из enum BIGNUM_ERRORS в случае ошибки 
 */
int bignum_from_gmp(bignum_t *dest, const mpz_t src);

/* --------------------Функции экспорта-------------------- */

/**
 * @brief Экспортирует число bignum_t в виде числа разрядностью 1 байт
 * 
 * @param dest Число-приемник разрядностью 1 байт
 * @param dest_size Емкость приемника (длина массива)
 * @param src Число-источник bignum_t
 * @return Число записанных элементов или отрицательное значение 
 * из enum BIGNUM_ERRORS в случае ошибки 
 */
long bignum_to_ui8_array(uint8_t *dest, size_t dest_size, const bignum_t *src);
/**
 * @brief Экспортирует число bignum_t в виде числа разрядностью 4 байта
 * 
 * @param dest Число-приемник разрядностью 4 байта
 * @param dest_size Емкость приемника (длина массива)
 * @param src Число-источник bignum_t
 * @return Число записанных элементов или отрицательное значение 
 * из enum BIGNUM_ERRORS в случае ошибки 
 */
long bignum_to_ui32_array(uint32_t *dest, size_t dest_size, const bignum_t *src);
/**
 * @brief Экспортирует число bignum_t в виде hex строки
 * 
 * @param dest Строка
 * @param dest_size Длина строки
 * @param src Число-источник bignum_t
 * @return Число записанных элементов или отрицательное значение 
 * из enum BIGNUM_ERRORS в случае ошибки 
 */
long bignum_to_hex_string(char *dest, size_t dest_size, const bignum_t *src);
/**
 * @brief Экспортирует число bignum_t в виде числа mpz_t
 * 
 * @param dest Число-приемник mpz_t
 * @param src Число-источник bignum_t
 */
void bignum_to_gmp(mpz_t dest, const bignum_t *src);

/* --------------------Служебные функции-------------------- */

/**
 * @brief Возвращает текущую разрядность числа
 * 
 * @param val Число типа bignum_t
 * @return Разрядность числа 
 */
size_t bignum_length(const bignum_t *val);
/**
 * @brief Возвращает максимальную разрядность числа
 * 
 * @param val Число типа bignum_t
 * @return Максимальная разрядность числа 
 */
size_t bignum_capacity(const bignum_t *val);
/**
 * @brief Проверяет число на равенство нулю
 * 
 * @param val Число типа bignum_t
 * @return Является ли число нулем или нет
 */
bool bignum_is_zero(const bignum_t *val);
/**
 * @brief Проверяет число на четность
 * 
 * @param val Число типа bignum_t
 * @return Является ли число четным или нет
 */
bool bignum_is_even(const bignum_t *val);

/* --------------------Арифметические операции и математические функции-------------------- */

/**
 * @brief Знаковое сравнение двух чисел
 * 
 * @param val_1 Первое число
 * @param val_2 Второе число
 * @return 1 - первое число больше, -1 - второе число больше,
 * 0 - равенство чисел
 */
int bignum_cmp(const bignum_t *val_1, const bignum_t *val_2);
/**
 * @brief Знаковое сложение двух чисел
 * 
 * @param res   Результат сложения
 * @param val_1 Первое число
 * @param val_2 Второе число
 * @return 0 в случае успеха, отрицательное значение 
 * из enum BIGNUM_ERRORS в случае ошибки 
 */
int bignum_add(bignum_t *res, const bignum_t *val_1, const bignum_t *val_2);
/**
 * @brief Знаковое вычитание двух чисел
 * 
 * @param res   Результат вычитания 
 * @param val_1 Первое число
 * @param val_2 Второе число
 * @return 0 в случае успеха, отрицательное значение 
 * из enum BIGNUM_ERRORS в случае ошибки 
 */
int bignum_sub(bignum_t *res, const bignum_t *val_1, const bignum_t *val_2);
/**
 * @brief Знаковое умножение двух чисел
 * 
 * @param res   Результат умножения
 * @param val_1 Первое число
 * @param val_2 Второе число
 * @return 0 в случае успеха, отрицательное значение 
 * из enum BIGNUM_ERRORS в случае ошибки 
 */
int bignum_mul(bignum_t *res, const bignum_t *val_1, const bignum_t *val_2);
/**
 * @brief Знаковое деление двух чисел методом Кнута
 * 
 * @param quot  Частное
 * @param rem   Остаток
 * @param val_1 Первое число
 * @param val_2 Второе число
 * @return 0 в случае успеха, отрицательное значение 
 * из enum BIGNUM_ERRORS в случае ошибки 
 */
int bignum_divmod(bignum_t *quot, bignum_t *rem, const bignum_t *val_1, const bignum_t *val_2);
/**
 * @brief Целая часть от деления двух чисел
 * 
 * @param quot  Частное
 * @param val_1 Первое число
 * @param val_2 Второе число
 * @return 0 в случае успеха, отрицательное значение 
 * из enum BIGNUM_ERRORS в случае ошибки 
 */
int bignum_div(bignum_t *quot, const bignum_t *val_1, const bignum_t *val_2);
/**
 * @brief Остаток от деления двух чисел
 * 
 * @param rem   Остаток от деления
 * @param val_1 Первое число
 * @param val_2 Второе число
 * @return 0 в случае успеха, отрицательное значение 
 * из enum BIGNUM_ERRORS в случае ошибки 
 */
int bignum_mod(bignum_t *rem, const bignum_t *val_1, const bignum_t *val_2);
/**
 * @brief Возведение в степень по модулю
 * 
 * @param res   Результат возведения в степень
 * @param base  Основание числа
 * @param exp   Степень числа
 * @param mod   Модуль
 * @return 0 в случае успеха, отрицательное значение 
 * из enum BIGNUM_ERRORS в случае ошибки 
 */
int bignum_pow_mod(bignum_t *res, const bignum_t *base, const bignum_t *exp, const bignum_t *mod);
/**
 * @brief Возведение в степень
 * 
 * @param res   Результат возведения в степень
 * @param base  Основание числа
 * @param exp   Степень числа
 * @return 0 в случае успеха, отрицательное значение 
 * из enum BIGNUM_ERRORS в случае ошибки 
 */
int bignum_pow(bignum_t *res, const bignum_t *base, const bignum_t *exp);
/**
 * @brief Нахождение НОД двух чисел
 * 
 * @param res   НОД
 * @param val_1 Первое число
 * @param val_2 Второе число
 * @return 0 в случае успеха, отрицательное значение 
 * из enum BIGNUM_ERRORS в случае ошибки 
 */
int bignum_gcd(bignum_t *res, const bignum_t *val_1, const bignum_t *val_2);
/**
 * @brief Нахождение НОК двух чисел
 * 
 * @param res   НОК
 * @param val_1 Первое число
 * @param val_2 Второе число
 * @return 0 в случае успеха, отрицательное значение 
 * из enum BIGNUM_ERRORS в случае ошибки 
 */
int bignum_lcm(bignum_t *res, const bignum_t *val_1, const bignum_t *val_2);
