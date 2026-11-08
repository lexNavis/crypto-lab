#include "../bignum/bignum.h"
/**
 * @brief Тест Миллера-Рабина для проверки числа на простоту
 *
 * @param[out] is_prime Результат: true — вероятно простое, false — составное
 * @param[in]  val      Проверяемое число (нечётное, > 3)
 * @param[in]  base     База (2 <= a <= n-2)
 * @return 0 в случае успеха, отрицательное значение из enum BIGNUM_ERRORS при ошибке
 */
int mersen_miller_rabin(bool *is_prime, const bignum_t *val, const bignum_t *base);
/**
 * @brief Сильный тест Люка на псевдопростоту
 *
 * @param[out] is_prime Результат: true — вероятно простое, false — составное
 * @param[in]  val        Проверяемое число (нечётное, > 3, не полный квадрат)
 * @return 0 в случае успеха, отрицательное значение из enum BIGNUM_ERRORS при ошибке
 */
int primality_lucas_strong(bool *is_prime, const bignum_t *val);

int primality_bpsw(bool *is_prime, const bignum_t *val);
