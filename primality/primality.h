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
/**
 * @brief Детерминированный тест Миллера на простоту.
 *
 * Для n < 2^64 использует фиксированный набор баз (2, 3, 5, 7, 11, 13,
 * 17, 19, 23, 29, 31, 37), для которого доказано: если n прошло все
 * базы, то n гарантированно простое (без вероятностной ошибки).
 *
 * Для n >= 2^64 детерминированного конечного набора баз не существует,
 * поэтому функция возвращает 1 («не применимо»).
 *
 * @param[out] is_prime Результат: true — простое, false — составное
 * @param[in]  n        Проверяемое число
 * @return 0 в случае успеха, 1 если n >= 2^64 (не применимо),
 *         отрицательное значение из enum BIGNUM_ERRORS при ошибке
 */
int primality_miller_deterministic(bool *is_prime, const bignum_t *n);
