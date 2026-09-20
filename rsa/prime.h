#ifndef PRIME_H
#define PRIME_H
#include <gmp.h>

#define PRIME_TEST_CYCLES 25

enum PRIME_ERRORS {
  ERR_INVALID_BITS = 1,
  ERR_SEED_FAILED,
  ERR_BITS_TOO_BIG
};

/**
 * @brief Создает простое число заданного размера
 *        и сохраняет его.
 * @param [val] Хранилище для числа (предварительно инициализированное)
 * @param [bits] Размер числа в битах
 * @return 0 - в случае успеха, отрицательные значения - ошибка
 */
int generate_prime(mpz_t val, unsigned int bits);
#endif /* PRIME_H */