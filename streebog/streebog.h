#ifndef STREEBOG_H
#define STREEBOG_H
#include <stdint.h>
#include <stddef.h>
#include "../bignum/bignum.h"
/**
 * @brief Тип для описания раундовых ключей "Стрибог"
 */
typedef uint8_t streebog_key_t[13][64];
/**
 * @brief Хэширует переданное сообщение
 * @param out Буфер для хранения хэш-кода
 * @param M Исходное сообщение
 * @param len Длина массива (число байт)
 * @returns 0 В случае успеха, отрицательное значение - ошибка
 * @note Всегда возвращает ноль. Что ж, пока так
 */
int streebog_make_hash(uint8_t out[64], const uint8_t *M, size_t len);
/**
 * @brief Обертка над streebog_make_hash для совместимости с bignum_t
 * @param res Результат выполнения хэширования
 * @param msg Исходное сообщение
 * @returns 0 В случае успеха, отрицательное значение - ошибка
 * @note Не работает, есть проблемы с преобразованием в uint8_t
 */
int bignum_streebog_hash(bignum_t *res, const bignum_t *msg);
/**
 * @brief Печать в консоль числа в hex в формате big-endian
 * @param buf Байтовый массив
 * @param len Длина 
 * @note Ожидается, что buf содержит байты в формате little-endian,
 * так как печатает массив задом наперед
 */
void print_hex(const uint8_t *buf, size_t len);
#endif /* STREEBOG_H */



