#ifndef RSA_H
#define RSA_H
#include <gmp.h>
#include <stdbool.h>

enum RSA_ERRORS {
  ERR_SNPRINTF_FAILED = 1,
  ERR_BUFFER_TOO_SMALL,
  ERR_RSA_SIZE_TOO_SMALL,
  ERR_FILE_NOT_FOUND,
  ERR_READ_FROM_FILE_FAILED,
  ERR_WRITE_TO_FILE_FAILED,
  ERR_EQUAL_PRIMES, /* Rare pokemon. Definetely :)*/
  ERR_INVERT_FAILED,
  ERR_MSG_TOO_BIG,
};

typedef struct
{
  mpz_t n; /* Modulus n = p * q*/
  mpz_t e; /* Public exp = 65537 by default */
  mpz_t d; /* Secret exp = e^(-1) mod (fi(n)) */
} rsa_key_t;
/**
 * @brief Первичная инициализация rsa_key_t
 * @param [key] Переменная rsa_key_t
 */
void rsa_key_init(rsa_key_t *key);
/**
 * @brief Освобождение памяти rsa_key_t
 * @param [key] Переменная rsa_key_t
 */
void rsa_key_clear(rsa_key_t *key);
/**
 * @brief Создает ключ RSA заданного размера
 * @param [key] Ключ RSA, в который будкт заносится информация
 * @param [bits] Число бит, из которого будет состоять ключ RSA
 * @param [from_file] Использовать ли простые числа из файла
 * @return 0 в случае успеха, отрицательное значение - в случае ошибки
 */
int rsa_keygen(rsa_key_t *key, unsigned int bits, bool from_file);
/**
 * @brief Осуществляет кодирование
 * @param [val] Исходное значение
 * @param [encrypted_val] Хранилище для результата
 * @param [key] Ключ RSA
 * @return 0 в случае успеха, отрицательное значение - в случае ошибки
 */
int rsa_encrypt(const mpz_t val, mpz_t encrypted_val, const rsa_key_t *key);
/**
 * @brief Осуществляет декодирование
 * @param [val] Закодированное значение
 * @param [decrypted_val] Хранилище для результата
 * @param [key] Ключ RSA
 * @return 0 в случае успеха, отрицательное значение - в случае ошибки
 */
int rsa_decrypt(const mpz_t val, mpz_t decrypted_val, const rsa_key_t *key);
#endif /* RSA_H */
