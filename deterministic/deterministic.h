#include "../bignum/bignum.h"
/**
 * @brief Детерминированный тест Миллера на простоту.
 *
 * @param[out] is_prime Результат: true — простое, false — составное
 * @param[in]  n        Проверяемое число
 * @return 0 в случае успеха, 1 если n >= 2^64 (не применимо),
 *         отрицательное значение из enum BIGNUM_ERRORS при ошибке
 */
int primality_miller_deterministic(bool *is_prime, const bignum_t *n);
/**
 * @brief Детерминированный тест AKS (Agrawal-Kayal-Saxena) на простоту.
 *
 * @param[out] is_prime Результат: true — простое, false — составное
 * @param[in]  n        Проверяемое число (n >= 2)
 * @return 0 в случае успеха, отрицательное значение из enum BIGNUM_ERRORS
 */
int primality_aks(bool *is_prime, const bignum_t *n);