#pragma once
#include "../bignum/bignum.h"
#include <stddef.h>

/**
 * @struct Полином над Z_n степени < r.
 *
 * Представление: массив из r коэффициентов (bignum_t), где coeffs[i] —
 * коэффициент при x^i. Степень полинома всегда < r, потому что все
 * операции выполняются по модулю x^r - 1 (т.е. x^r ≡ 1).
 *
 * Например, для r = 4 полином x^5 + 3x^2 + 1 после редукции по x^4 - 1
 * станет x^(5 mod 4) + 3x^2 + 1 = x + 3x^2 + 1, т.е. coeffs[1] = 1,
 * coeffs[2] = 3, coeffs[0] = 1, coeffs[3] = 0.
 *
 * Все коэффициенты хранятся в [0, n-1] (неотрицательные остатки по mod n).
 */
typedef struct poly_t
{
  bignum_t *coeffs;  /* Коэффициенты, длина r */
  size_t    r;       /* Модуль редукции x^r - 1 */
} poly_t;

/**
 * @brief Инициализирует полином: выделяет память под r коэффициентов,
 * все коэффициенты = 0.
 *
 * @param[out] p Полином
 * @param[in]  r Модуль редукции (степень x^r - 1)
 * @return 0 в случае успеха, отрицательное значение из enum BIGNUM_ERRORS
 */
int poly_init(poly_t *p, size_t r);

/**
 * @brief Освобождает память полинома.
 *
 * @param[in,out] p Полином
 */
void poly_delete(poly_t *p);

/**
 * @brief Устанавливает коэффициент при x^deg в значение val (mod n).
 *
 * @param[in,out] p   Полином
 * @param[in]     deg Степень (0 <= deg < r)
 * @param[in]     val Значение коэффициента (будет приведено по mod n)
 * @param[in]     n   Модуль
 * @return 0 в случае успеха, отрицательное значение из enum BIGNUM_ERRORS
 */
int poly_set_coeff(poly_t *p, size_t deg, const bignum_t *val, const bignum_t *n);

/**
 * @brief Копирует полином src в dest.
 *
 * @param[out] dest Полином-приёмник (должен быть инициализирован с тем же r)
 * @param[in]  src  Полином-источник
 * @return 0 в случае успеха, отрицательное значение из enum BIGNUM_ERRORS
 */
int poly_copy(poly_t *dest, const poly_t *src);

/**
 * @brief Сравнивает два полинома на равенство (все коэффициенты равны).
 *
 * @param[in] a Первый полином
 * @param[in] b Второй полином
 * @return true если равны, false иначе
 */
bool poly_equal(const poly_t *a, const poly_t *b);

/**
 * @brief Сложение полиномов по модулю x^r - 1 и mod n.
 *
 * @param[out] res Результат (должен быть инициализирован с тем же r)
 * @param[in]  a   Первый полином
 * @param[in]  b   Второй полином
 * @param[in]  n   Модуль для коэффициентов
 * @return 0 в случае успеха, отрицательное значение из enum BIGNUM_ERRORS
 */
int poly_add(poly_t *res, const poly_t *a, const poly_t *b, const bignum_t *n);

/**
 * @brief Вычитание полиномов по модулю x^r - 1 и mod n.
 *
 * @param[out] res Результат
 * @param[in]  a   Первый полином
 * @param[in]  b   Второй полином
 * @param[in]  n   Модуль для коэффициентов
 * @return 0 в случае успеха, отрицательное значение из enum BIGNUM_ERRORS
 */
int poly_sub(poly_t *res, const poly_t *a, const poly_t *b, const bignum_t *n);

/**
 * @brief Умножение полиномов по модулю x^r - 1 и mod n.
 *
 * Редукция по x^r - 1: член x^(i+j) при i+j >= r превращается в x^(i+j-r),
 * потому что x^r ≡ 1.
 *
 * @param[out] res Результат (должен быть инициализирован, все коэффициенты 0)
 * @param[in]  a   Первый полином
 * @param[in]  b   Второй полином
 * @param[in]  n   Модуль для коэффициентов
 * @return 0 в случае успеха, отрицательное значение из enum BIGNUM_ERRORS
 */
int poly_mul(poly_t *res, const poly_t *a, const poly_t *b, const bignum_t *n);

/**
 * @brief Возведение полинома в степень exp по модулю x^r - 1 и mod n.
 * Использует бинарное возведение в степень.
 *
 * @param[out] res Результат (должен быть инициализирован, все коэффициенты 0)
 * @param[in]  base Основание
 * @param[in]  exp  Показатель (bignum_t, неотрицательный)
 * @param[in]  n    Модуль для коэффициентов
 * @return 0 в случае успеха, отрицательное значение из enum BIGNUM_ERRORS
 */
int poly_pow(poly_t *res, const poly_t *base, const bignum_t *exp, const bignum_t *n);