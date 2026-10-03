#ifndef KUZNYECHIK_H
#define KUZNYECHIK_H
#include <stdint.h>
#include <stddef.h>
#include "../bignum/bignum.h"
/**
 * @brief Структура для описания ключа шифрования "Кузнечик"
 */
typedef struct {
    uint8_t k[32];
    uint8_t k_arr[10][16];
}kuz_key_t;
/**
 * @brief Создание ключа для шифра "Кузнечик". Ключ генерируется внутри функции
 * @param key Хранилище для нового ключа
 * @return 0 - Успех, отрицательное значение - ошибка
 */
int kuz_keygen(kuz_key_t *key);
/**
 * @brief Использование заданного ключа для шифра "Кузнечик".
 * @param key Хранилище для нового ключа
 * @return 0 - Успех, отрицательное значение - ошибка
 */
int kuz_keygen_from_raw(kuz_key_t *key, const uint8_t raw[32]);
/**
 * @brief Шифрование блока 128 бит
 * @param block Исходный блок (фрагмент сообщения)
 * @param key Ключ шифрования
 * @return 0 - Успех, отрицательное значение - ошибка
 */
int kuz_encrypt(uint8_t block[16], const kuz_key_t *key);
/**
 * @brief Дешифровка блока 128 бит
 * @param [block] Указатель на блок (16 байт)
 * @param [key] Ключ шифрования
 * @return 0 - Успех, отрицательное значение - ошибка
 */
int kuz_decrypt(uint8_t block[16], const kuz_key_t *key);
/**
 * @brief Обертка над kuz_encrypt для совместимости с bignum_t
 * @param res Результат выполнения кодирования
 * @param block Исходное сообщение
 * @returns 0 в случае успеха, отрицательное значение - ошибка
 * @note Работает только на сообщениях, кратных 16 байтам, проблемы с ведущими нулями
 */
int bignum_kuz_encrypt(bignum_t *res, const bignum_t *block, const kuz_key_t *key);
/**
 * @brief Обертка над kuz_decrypt для совместимости с bignum_t
 * @param res Результат выполнения декодирования
 * @param block Зашифрованное сообщение
 * @returns 0 в случае успеха, отрицательное значение - ошибка
 * @note Работает только на сообщениях, кратных 16 байтам, проблемы с ведущими нулями
 */
int bignum_kuz_decrypt(bignum_t *res, const bignum_t *block, const kuz_key_t *key);

#endif /* KUZNYECHIK_H */



