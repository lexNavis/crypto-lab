#include "poly.h"
#include <stdlib.h>
#include <string.h>

int poly_init(poly_t *p, size_t r)
{
  if (!p || r == 0)
    return -ERR_INVALID_PARAM;

  p->coeffs = calloc(r, sizeof(bignum_t));
  if (!p->coeffs)
    return -ERR_MALLOC_FAILED;

  p->r = r;

  /* Инициализируем каждый коэффициент как bignum = 0 */
  for (size_t i = 0; i < r; i++)
    bignum_init(&p->coeffs[i]);

  return 0;
}

void poly_delete(poly_t *p)
{
  if (!p || !p->coeffs)
    return;
  for (size_t i = 0; i < p->r; i++)
    bignum_delete(&p->coeffs[i]);
  free(p->coeffs);
  p->coeffs = NULL;
  p->r = 0;
}

int poly_set_coeff(poly_t *p, size_t deg, const bignum_t *val, const bignum_t *n)
{
  if (!p || !val || !n)
    return -ERR_NULLPTR;
  if (deg >= p->r)
    return -ERR_INVALID_PARAM;
  return bignum_mod(&p->coeffs[deg], val, n);
}

int poly_copy(poly_t *dest, const poly_t *src)
{
  if (!dest || !src)
    return -ERR_NULLPTR;
  if (dest->r != src->r)
    return -ERR_INVALID_PARAM;

  for (size_t i = 0; i < src->r; i++)
  {
    int err = bignum_copy(&dest->coeffs[i], &src->coeffs[i]);
    if (err < 0)
      return err;
  }
  return 0;
}

bool poly_equal(const poly_t *a, const poly_t *b)
{
  if (!a || !b || a->r != b->r)
    return false;
  for (size_t i = 0; i < a->r; i++)
  {
    if (bignum_cmp(&a->coeffs[i], &b->coeffs[i]) != 0)
      return false;
  }
  return true;
}

int poly_add(poly_t *res, const poly_t *a, const poly_t *b, const bignum_t *n)
{
  if (!res || !a || !b || !n)
    return -ERR_NULLPTR;
  if (res->r != a->r || a->r != b->r)
    return -ERR_INVALID_PARAM;

  for (size_t i = 0; i < a->r; i++)
  {
    int err = bignum_add(&res->coeffs[i], &a->coeffs[i], &b->coeffs[i]);
    if (err < 0)
      return err;
    err = bignum_mod(&res->coeffs[i], &res->coeffs[i], n);
    if (err < 0)
      return err;
  }
  return 0;
}

int poly_sub(poly_t *res, const poly_t *a, const poly_t *b, const bignum_t *n)
{
  if (!res || !a || !b || !n)
    return -ERR_NULLPTR;
  if (res->r != a->r || a->r != b->r)
    return -ERR_INVALID_PARAM;

  for (size_t i = 0; i < a->r; i++)
  {
    int err = bignum_sub(&res->coeffs[i], &a->coeffs[i], &b->coeffs[i]);
    if (err < 0)
      return err;
    /* Приводим к неотрицательному остатку [0, n-1]: bignum_mod для
     * отрицательного результата даёт отрицательный остаток (знак
     * сохраняется как у делимого), поэтому при отрицательном добавляем n. */
    err = bignum_mod(&res->coeffs[i], &res->coeffs[i], n);
    if (err < 0)
      return err;
    if (res->coeffs[i].negative)
    {
      err = bignum_add(&res->coeffs[i], &res->coeffs[i], n);
      if (err < 0)
        return err;
    }
  }
  return 0;
}

int poly_mul(poly_t *res, const poly_t *a, const poly_t *b, const bignum_t *n)
{
  if (!res || !a || !b || !n)
    return -ERR_NULLPTR;
  if (res->r != a->r || a->r != b->r)
    return -ERR_INVALID_PARAM;

  size_t r = a->r;

  /* Обнуляем результат */
  for (size_t i = 0; i < r; i++)
  {
    int err = bignum_set_zero(&res->coeffs[i]);
    if (err < 0)
      return err;
  }

  /* Стандартное умножение полиномов O(r^2):
   * res[i+j] += a[i] * b[j], с редукцией x^(i+j) -> x^((i+j) mod r)
   * (потому что x^r ≡ 1 по модулю x^r - 1). */
  bignum_t prod, tmp;
  bignum_init(&prod);
  bignum_init(&tmp);

  for (size_t i = 0; i < r; i++)
  {
    if (bignum_is_zero(&a->coeffs[i]))
      continue;  /* пропускаем нулевые коэффициенты — экономия */

    for (size_t j = 0; j < r; j++)
    {
      if (bignum_is_zero(&b->coeffs[j]))
        continue;

      int err = bignum_mul(&prod, &a->coeffs[i], &b->coeffs[j]);
      if (err < 0)
        goto cleanup;

      /* Редукция по x^r - 1: x^(i+j) = x^((i+j) mod r) */
      size_t deg = (i + j) % r;

      err = bignum_add(&tmp, &res->coeffs[deg], &prod);
      if (err < 0)
        goto cleanup;
      err = bignum_mod(&res->coeffs[deg], &tmp, n);
      if (err < 0)
        goto cleanup;
    }
  }

  int err = 0;
cleanup:
  bignum_delete(&prod);
  bignum_delete(&tmp);
  return err;
}

int poly_pow(poly_t *res, const poly_t *base, const bignum_t *exp, const bignum_t *n)
{
  if (!res || !base || !exp || !n)
    return -ERR_NULLPTR;
  if (res->r != base->r)
    return -ERR_INVALID_PARAM;

  /* Результат = 1 (полином-константа 1) */
  bignum_t one;
  bignum_init(&one);
  int err = bignum_from_ui32_array(&one, (uint32_t[]){1}, 1, false);
  if (err < 0) { bignum_delete(&one); return err; }

  for (size_t i = 0; i < res->r; i++)
  {
    err = bignum_set_zero(&res->coeffs[i]);
    if (err < 0) { bignum_delete(&one); return err; }
  }
  err = bignum_copy(&res->coeffs[0], &one);
  bignum_delete(&one);
  if (err < 0) return err;

  /* Бинарное возведение в степень: идём по битам exp с младшего.
   * Если бит 1 — res *= base. base = base^2 на каждом шаге. */
  bignum_t e;
  bignum_init(&e);
  err = bignum_copy(&e, exp);
  if (err < 0) { bignum_delete(&e); return err; }

  poly_t base_copy, tmp;
  err = poly_init(&base_copy, base->r);
  if (err < 0) { bignum_delete(&e); return err; }
  err = poly_init(&tmp, base->r);
  if (err < 0) { bignum_delete(&e); poly_delete(&base_copy); return err; }

  err = poly_copy(&base_copy, base);
  if (err < 0) goto cleanup;

  while (!bignum_is_zero(&e))
  {
    if (!bignum_is_even(&e))
    {
      err = poly_mul(&tmp, res, &base_copy, n);
      if (err < 0) goto cleanup;
      err = poly_copy(res, &tmp);
      if (err < 0) goto cleanup;
    }
    bignum_shift_right_bits(&e, 1);

    if (!bignum_is_zero(&e))
    {
      err = poly_mul(&tmp, &base_copy, &base_copy, n);
      if (err < 0) goto cleanup;
      err = poly_copy(&base_copy, &tmp);
      if (err < 0) goto cleanup;
    }
  }
  err = 0;

cleanup:
  poly_delete(&base_copy);
  poly_delete(&tmp);
  bignum_delete(&e);
  return err;
}
