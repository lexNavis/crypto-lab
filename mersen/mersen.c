#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "mersen.h"

/* Переводит bignum_t в uint64_t. Возвращает 0 при успехе,
 * 1 если не влезает в uint64_t, отрицательное при ошибке. */
static int bignum_to_ui64(uint64_t *out, const bignum_t *val)
{
  size_t len = bignum_length(val);
  if (len > 2)
    return 1;
  uint32_t words[2] = {0, 0};
  long got = bignum_to_ui32_array(words, len, val);
  if (got < 0)
    return (int)got;
  *out = (got == 2) ? (((uint64_t)words[1] << 32) | words[0]) : words[0];
  return 0;
}

int form_mersen_number(bignum_t *val, uint32_t exp)
{
  if (!val)
  {
    return -ERR_NULLPTR;
  }
  if (exp == 0)
  {
    return -ERR_INVALID_PARAM;
  }
  int err = bignum_from_ui8_array(val, (uint8_t[]){1}, 1, 0);
  if (err)
  {
    return err;
  }
  /* Сдвиг на exp бит */
  err = bignum_shift_left_bits(val, (size_t)exp);
  if (err)
  {
    return err;
  }
  bignum_t one;
  bignum_init(&one);
  err = bignum_from_ui8_array(&one, (uint8_t[]){1}, 1, 0);
  if (err)
  {
    goto cleanup;
  }
  /* Вычитание единицы */
  err = bignum_sub(val, val, &one);
  if (err)
  {
    goto cleanup;
  }
cleanup:
  bignum_delete(&one);
  return err;
}

int mersen_check_standard(const bignum_t *val, bool *is_prime)
{
  int err = 0;
  if (!val || !is_prime)
    return -ERR_NULLPTR;

  bignum_t one, two, three, root, i, rem, two_bn;
  bignum_init(&one);
  bignum_init(&two);
  bignum_init(&three);
  bignum_init(&root);
  bignum_init(&i);
  bignum_init(&rem);
  bignum_init(&two_bn);

  err = bignum_from_ui32_array(&one, (uint32_t[]){1}, 1, false);
  if (err < 0)
    goto cleanup;
  err = bignum_from_ui32_array(&two, (uint32_t[]){2}, 1, false);
  if (err < 0)
    goto cleanup;
  err = bignum_from_ui32_array(&three, (uint32_t[]){3}, 1, false);
  if (err < 0)
    goto cleanup;

  /* 0 и 1 — не простые */
  if (bignum_is_zero(val) || bignum_cmp(val, &one) == 0)
  {
    *is_prime = false;
    err = 0;
    goto cleanup;
  }
  /* 2 и 3 — простые */
  if (bignum_cmp(val, &two) == 0 || bignum_cmp(val, &three) == 0)
  {
    *is_prime = true;
    err = 0;
    goto cleanup;
  }
  /* Чётность */
  if (bignum_is_even(val))
  {
    *is_prime = false;
    err = 0;
    goto cleanup;
  }
  /* Признаки делимости: 3, 5, 6, 9 */
  uint32_t r = 0;
  err = bignum_mod_ui32(&r, val, 3);
  if (err < 0)
    goto cleanup;
  if (r == 0)
  {
    *is_prime = false;
    err = 0;
    goto cleanup;
  }

  err = bignum_mod_ui32(&r, val, 5);
  if (err < 0)
    goto cleanup;
  if (r == 0)
  {
    *is_prime = false;
    err = 0;
    goto cleanup;
  }

  err = bignum_mod_ui32(&r, val, 6);
  if (err < 0)
    goto cleanup;
  if (r == 0)
  {
    *is_prime = false;
    err = 0;
    goto cleanup;
  }

  err = bignum_mod_ui32(&r, val, 9);
  if (err < 0)
    goto cleanup;
  if (r == 0)
  {
    *is_prime = false;
    err = 0;
    goto cleanup;
  }

  /* Перебор делителей от 3 до sqrt(val) шагом 2 */
  err = bignum_sqrt(&root, val);
  if (err < 0)
    goto cleanup;

  err = bignum_from_ui32_array(&i, (uint32_t[]){3}, 1, false);
  if (err < 0)
    goto cleanup;
  err = bignum_from_ui32_array(&two_bn, (uint32_t[]){2}, 1, false);
  if (err < 0)
    goto cleanup;

  while (bignum_cmp(&i, &root) <= 0)
  {
    err = bignum_mod(&rem, val, &i);
    if (err < 0)
      goto cleanup;
    if (bignum_is_zero(&rem))
    {
      *is_prime = false;
      err = 0;
      goto cleanup;
    }
    err = bignum_add(&i, &i, &two_bn);
    if (err < 0)
      goto cleanup;
  }
  *is_prime = true;
  err = 0;

cleanup:
  bignum_delete(&one);
  bignum_delete(&two);
  bignum_delete(&three);
  bignum_delete(&root);
  bignum_delete(&i);
  bignum_delete(&rem);
  bignum_delete(&two_bn);
  return err;
}

int mersen_check_eratosthenes(const bignum_t *val, bool *is_prime)
{
  if (!val)
  {
    return -ERR_NULLPTR;
  }
  bignum_t root;
  bignum_init(&root);
  int err = bignum_sqrt(&root, val);
  if (err < 0)
  {
    bignum_delete(&root);
    return err;
  }
  /* Так как для выделения памяти требуется size_t, то значение
   * корня должно влезать в uint64_t. Такое вот ограничение */
  uint64_t r = 0;
  err = bignum_to_ui64(&r, &root);
  if (err < 0)
  {
    bignum_delete(&root);
    return err;
  }
  if (err > 0)
  {
    err = -ERR_TOO_MUCH_MEMORY; // не влезает в uint64_t
    bignum_delete(&root);
    return err;
  }
  if (r > (1ULL << 30))
  {
    err = -ERR_TOO_MUCH_MEMORY;
    bignum_delete(&root);
    return err;
  }
  /* Первичная инициализация решета;
   * Индекс - число, значение - является ли оно составным.
   * По умолчанию, все числа не изучены (false) */
  bool *sieve = calloc(r + 1, sizeof(bool));
  if (!sieve)
  {
    bignum_delete(&root);
    return -ERR_MALLOC_FAILED;
  }
  /* Суть метода: заранее игнорировать составные числа
   * при переборе делителей до корня.
   * Если число x простое, то делится на 1 и
   * * на само себя, поэтому x^2 - составное.
   * Так же составными являются x^2 + nx, так как
   * дают x + n  с нулевым остатком.*/
  /* 1. Построить решето полностью */
  for (size_t i = 2; i * i <= r; i++)
  {
    if (!sieve[i])
    {
      for (size_t j = i * i; j <= r; j += i)
      {
        sieve[j] = true;
      }
    }
  }

  /* 2. Проверить N на делимость на простые */
  bool prime = true;
  for (size_t i = 2; i <= r; i++)
  {
    if (!sieve[i])
    {
      bignum_t div, rem;
      bignum_init(&div);
      bignum_init(&rem);
      uint32_t words[2] = {(uint32_t)i, (uint32_t)(i >> 32)};
      size_t len = (words[1] != 0) ? 2 : 1;
      err = bignum_from_ui32_array(&div, words, len, false);
      if (err < 0)
      {
        bignum_delete(&div);
        bignum_delete(&rem);
        free(sieve);
        bignum_delete(&root);
        return err;
      }
      err = bignum_mod(&rem, val, &div);
      bignum_delete(&div);
      if (err < 0)
      {
        bignum_delete(&rem);
        free(sieve);
        bignum_delete(&root);
        return err;
      }
      if (bignum_is_zero(&rem))
      {
        prime = false;
        bignum_delete(&rem);
        break;
      }
      bignum_delete(&rem);
    }
  }
  *is_prime = prime;
  err = 0;
  free(sieve);
  bignum_delete(&root);
  return err;
}

/* Инвертирует кандидата sieve[idx], если res <= root и res % 12 == mod1 или mod2.
 * Если mod2 == 0 — проверяется только mod1. */
static int atkin_mark(bignum_t *res, const bignum_t *root, uint32_t mod1, uint32_t mod2, bool *sieve)
{
  if (bignum_cmp(res, root) == 1)
    return 0; /* res > root */
  uint32_t mod12;
  int err = bignum_mod_ui32(&mod12, res, 12);
  if (err < 0)
    return err;
  if (mod12 == mod1 || (mod2 != 0 && mod12 == mod2))
  {
    uint64_t idx = 0;
    int rc = bignum_to_ui64(&idx, res);
    if (rc < 0)
      return rc;
    if (rc == 0)
      sieve[idx] = !sieve[idx];
  }
  return 0;
}

int mersen_check_atkin(const bignum_t *val, bool *is_prime)
{
  if (!val)
  {
    return -ERR_NULLPTR;
  }
  bignum_t root;
  bignum_t x, y, res, tmpx, tmpy;
  bignum_init(&x);
  bignum_init(&y);
  bignum_init(&res);
  bignum_init(&tmpx);
  bignum_init(&tmpy);
  bignum_init(&root);
  int err = bignum_sqrt(&root, val);
  if (err < 0)
  {
    goto cleanup;
  }
  /* Так как для выделения памяти требуется size_t, то значение
   * корня должно влезать в uint64_t. Такое вот ограничение */
  uint32_t words[2] = {0, 0};
  long got = bignum_to_ui32_array(words, bignum_length(&root), &root);
  if (got < 0)
  {
    err = -ERR_INIT;
    goto cleanup;
  }
  uint64_t r = (got == 2) ? (((uint64_t)words[1] << 32) | words[0]) : words[0];
  if (r > (1ULL << 30))
  {
    err = -ERR_TOO_MUCH_MEMORY;
    goto cleanup;
  }
  /* Первичная инициализация решета;
   * Индекс - число, значение - является ли оно кандидатом
   * на простое число. По умолчанию, все числа не кандидаты (false) */
  bool *sieve = calloc(r + 1, sizeof(bool));
  if (!sieve)
  {
    err = -ERR_MALLOC_FAILED;
    goto cleanup;
  }
  /* Суть метода аналогична решету Эратосфена,
   * но построение решета другое. */

  /* 1. Построить решето полностью */
  for (uint64_t i = 1; i * i <= r; i++)
  {
    for (uint64_t j = 1; j * j <= r; j++)
    {
      uint32_t x_words[2] = {(uint32_t)i, (uint32_t)(i >> 32)};
      size_t x_len = (x_words[1] != 0) ? 2 : 1;
      err = bignum_from_ui32_array(&x, x_words, x_len, false);
      if (err < 0)
        goto free_sieve;
      uint32_t y_words[2] = {(uint32_t)j, (uint32_t)(j >> 32)};
      size_t y_len = (y_words[1] != 0) ? 2 : 1;
      err = bignum_from_ui32_array(&y, y_words, y_len, false);
      if (err < 0)
        goto free_sieve;

      /* 1: res = 4x² + y², mod12 == 1 или 5 */
      err = bignum_mul(&tmpx, &x, &x);
      if (err < 0)
        goto free_sieve;
      err = bignum_mul_digit(&tmpx, &tmpx, 4);
      if (err < 0)
        goto free_sieve;
      err = bignum_mul(&tmpy, &y, &y);
      if (err < 0)
        goto free_sieve;
      err = bignum_add(&res, &tmpx, &tmpy);
      if (err < 0)
        goto free_sieve;
      err = atkin_mark(&res, &root, 1, 5, sieve);
      if (err < 0)
        goto free_sieve;

      /* 2: res = 3x² + y², mod12 == 7 */
      err = bignum_mul(&tmpx, &x, &x);
      if (err < 0)
        goto free_sieve;
      err = bignum_mul_digit(&tmpx, &tmpx, 3);
      if (err < 0)
        goto free_sieve;
      err = bignum_add(&res, &tmpx, &tmpy);
      if (err < 0)
        goto free_sieve;
      err = atkin_mark(&res, &root, 7, 0, sieve);
      if (err < 0)
        goto free_sieve;

      /* 3: res = 3x² − y², x > y, mod12 == 11 */
      if (bignum_cmp(&x, &y) == 1)
      {
        err = bignum_sub(&res, &tmpx, &tmpy);
        if (err < 0)
          goto free_sieve;
        err = atkin_mark(&res, &root, 11, 0, sieve);
        if (err < 0)
          goto free_sieve;
      }
    }
  }
  /* Отсеивание квадратов */
  for (uint64_t n = 5; n * n <= r; n++)
  {
    if (sieve[n])
    {
      for (uint64_t k = n * n; k <= r; k += n)
      {
        sieve[k] = false;
      }
    }
  }
  /* Особые случаи */
  if (r >= 2)
  {
    sieve[2] = true;
  }
  if (r >= 3)
  {
    sieve[3] = true;
  }
  /* 2. Проверить N на делимость на простые */
  bool prime = true;
  for (uint64_t i = 2; i <= r; i++)
  {
    if (!sieve[i])
      continue;
    bignum_t div, rem;
    bignum_init(&div);
    bignum_init(&rem);
    uint32_t words[2] = {(uint32_t)i, (uint32_t)(i >> 32)};
    size_t len = (words[1] != 0) ? 2 : 1;
    err = bignum_from_ui32_array(&div, words, len, false);
    if (err < 0)
    {
      bignum_delete(&div);
      bignum_delete(&rem);
      goto free_sieve;
    }
    err = bignum_mod(&rem, val, &div);
    bignum_delete(&div);
    if (err < 0)
    {
      bignum_delete(&rem);
      goto free_sieve;
    }
    if (bignum_is_zero(&rem))
    {
      prime = false;
      bignum_delete(&rem);
      break;
    }
    bignum_delete(&rem);
  }
  *is_prime = prime;
  err = 0;
free_sieve:
  free(sieve);
cleanup:
  bignum_delete(&x);
  bignum_delete(&y);
  bignum_delete(&res);
  bignum_delete(&tmpx);
  bignum_delete(&tmpy);
  bignum_delete(&root);
  return err;
}

static bool is_prime_u32(uint32_t val)
{
  if (val < 2)
    return false;
  for (uint32_t i = 2; i * i <= val; i++)
  {
    if (val % i == 0)
    {
      return false;
    }
  }
  return true;
}

int mersen_check_lucas_lehmer(const bignum_t *val, uint32_t exp, bool *is_prime)
{
  if (!val)
  {
    return -ERR_NULLPTR;
  }
  /* Проверим на простоту показатель степени.
   * Если он составной, то и число составное.
   * Особые случаи: 2: 4 - 1 = 3 */
  if (exp == 2)
  {
    *is_prime = true;
    return 0;
  }
  if (!is_prime_u32(exp))
  {
    *is_prime = false;
    return 0;
  }
  /* s = 4.
   * Повторить exp − 2 раз:
   * s = s * s.
   * s = s − 2.
   * s = s mod val.
   * В конце: если s == 0, тогда val простое. Иначе составное. */
  bignum_t s, two;
  bignum_init(&s);
  bignum_init(&two);
  int err = bignum_from_ui8_array(&s, (uint8_t[]){4}, 1, 0);
  if (err < 0)
  {
    goto cleanup;
  }
  err = bignum_from_ui8_array(&two, (uint8_t[]){2}, 1, 0);
  if (err < 0)
  {
    goto cleanup;
  }
  for (uint32_t i = 0; i < exp - 2; i++)
  {
    err = bignum_mul(&s, &s, &s);
    if (err < 0)
    {
      goto cleanup;
    }
    err = bignum_sub(&s, &s, &two);
    if (err < 0)
    {
      goto cleanup;
    }
    err = bignum_mod(&s, &s, val);
    if (err < 0)
    {
      goto cleanup;
    }
  }
  *is_prime = bignum_is_zero(&s);
  err = 0;
cleanup:
  bignum_delete(&s);
  bignum_delete(&two);
  return err;
}
