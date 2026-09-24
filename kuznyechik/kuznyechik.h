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

/**
 * @brief Печать 16 байтового числа
 * @param [blk] - массив 16 байт, содержащий число
 */
void print_block(uint8_t blk[16]);

void test_solo_arg(
    uint8_t out[16], 
    uint8_t args[][16],
    size_t count, 
    void(*func)(uint8_t*, uint8_t*)
);

void test_f(uint8_t arg_1[16], uint8_t arg_2[16], uint8_t module[16]);
#endif /* KUZNYECHIK_H */



