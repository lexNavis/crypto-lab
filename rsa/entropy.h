#ifndef ENTROPY_H
#define ENTROPY_H
#include <gmp.h>
#include <stddef.h>

enum ENTROPY_ERRORS {
  ERR_GET_RANDOM_FAIL_TOTAL = 1,
  ERR_GET_RANDOM_FAIL_PARTIAL,
};
/**
 * @brief Заполняет сид gmp
 * @param [state] Переменная - сид
 * @return 0 - в случае успеха, иные значения - ошибка
 */
int seed_gmp_randstate(gmp_randstate_t state);
#endif /* ENTROPY_H */