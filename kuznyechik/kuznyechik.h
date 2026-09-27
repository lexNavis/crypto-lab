#ifndef KUZNYECHIK_H
#define KUZNYECHIK_H
#include <stdint.h>
#include <stddef.h>
/**
 * @brief Структура для описания ключа шифрования "Кузнечик"
 */
typedef struct {
    uint8_t k[32];
    uint8_t k_arr[10][16];
}kuz_key_t;
/**
 * @brief Создание ключа для шифра "Кузнечик". Ключ генерируется внутри функции
 * @param [key] Хранилище для нового ключа
 * @return 0 - Успех, отрицательное значение - ошибка
 */
int kuz_keygen(kuz_key_t *key);

int kuz_keygen_from_raw(kuz_key_t *key, const uint8_t raw[32]);
/**
 * @brief Шифрование блока
 * @param [block] Указатель на блок (16 байт)
 * @param [key] Ключ шифрования
 * @return 0 - Успех, отрицательное значение - ошибка
 */
int kuz_encrypt(uint8_t block[16], const kuz_key_t *key);
/**
 * @brief Дешифровка блока
 * @param [block] Указатель на блок (16 байт)
 * @param [key] Ключ шифрования
 * @return 0 - Успех, отрицательное значение - ошибка
 */
int kuz_decrypt(uint8_t block[16], const kuz_key_t *key);

#endif /* KUZNYECHIK_H */



