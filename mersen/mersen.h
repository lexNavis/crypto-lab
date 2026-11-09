#pragma once
#include "../bignum/bignum.h"

/**
 * @brief Формирует число Мерсенна M = 2^exp - 1 в уже инициализированном val.
 *
 * @param[out] val Число-приёмник (инициализировано)
 * @param[in]  exp Показатель степени (exp > 0)
 * @return 0 в случае успеха, отрицательное значение из enum BIGNUM_ERRORS
 */
int form_mersen_number(bignum_t *val, uint32_t exp);

/**
 * @brief Проверка числа на простоту «стандартным» методом.
 *
 * @param[in]  val      Проверяемое число
 * @param[out] is_prime Результат: true — простое, false — составное
 * @return 0 в случае успеха, отрицательное значение из enum BIGNUM_ERRORS
 */
int mersen_check_standard(const bignum_t *val, bool *is_prime);

/**
 * @brief Проверка числа на простоту решетом Эратосфена.
 *
 * @param[in]  val      Проверяемое число
 * @param[out] is_prime Результат: true — простое, false — составное
 * @return 0 в случае успеха, 1 если val слишком велико для решета
 *         (не применимо), отрицательное значение из enum BIGNUM_ERRORS
 */
int mersen_check_eratosthenes(const bignum_t *val, bool *is_prime);

/**
 * @brief Проверка числа на простоту решетом Аткина.

 * @param[in]  val      Проверяемое число
 * @param[out] is_prime Результат: true — простое, false — составное
 * @return 0 в случае успеха, 1 если val слишком велико для решета
 *         (не применимо), отрицательное значение из enum BIGNUM_ERRORS
 */
int mersen_check_atkin(const bignum_t *val, bool *is_prime);

/**
 * @brief Проверка числа Мерсенна на простоту тестом Люка-Лемера.
 *
 * @param[in]  val      Число Мерсенна (2^exp - 1), уже сформированное
 * @param[in]  exp      Показатель степени (тот же, что при формировании val)
 * @param[out] is_prime Результат: true — простое, false — составное
 * @return 0 в случае успеха, отрицательное значение из enum BIGNUM_ERRORS
 */
int mersen_check_lucas_lehmer(const bignum_t *val, uint32_t exp, bool *is_prime);