#ifndef STREEBOG_H
#define STREEBOG_H
#include <stdint.h>
#include <stddef.h>
/**
 * @brief Тип для описания раундовых ключей "Стрибог"
 */
typedef uint8_t streebog_key_t[13][64];
/**
 * @brief Хэширует переданное сообщение
 * @param [out] - буфер для хранения хэш-кода
 * @param [M] - указатель на байтовый массив (Сообщение)
 * @param [len] - длина массива (число байт)
 * @returns 0 - в случае успеха, отрицательное значение - ошибка
 * @note Всегда возвращает ноль. Что ж, пока так
 */
int streebog_make_hash(uint8_t out[64], const uint8_t *M, size_t len);

void print_hex(const uint8_t *buf, size_t len);
#endif /* STREEBOG_H */



