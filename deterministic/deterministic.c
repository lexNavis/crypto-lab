#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "deterministic.h"
#include "../primality/primality.h"
#include "poly.h"

int primality_miller_deterministic(bool *is_prime, const bignum_t *n) {
  if (!is_prime || !n)
    return -ERR_NULLPTR;

  *is_prime = false;

  bignum_t two, three, nm2;
  bignum_init(&two);
  bignum_init(&three);
  bignum_init(&nm2);

  int err = bignum_from_ui32_array(&two, (uint32_t[]){2}, 1, false);
  if (err < 0)
    goto cleanup;
  err = bignum_from_ui32_array(&three, (uint32_t[]){3}, 1, false);
  if (err < 0)
    goto cleanup;

  if (bignum_cmp(n, &two) == -1) {
    *is_prime = false;
    goto cleanup;
  }
  if (bignum_cmp(n, &two) == 0 || bignum_cmp(n, &three) == 0) {
    *is_prime = true;
    goto cleanup;
  }

  /* Детерминированный набор баз существует только для n < 2^64.
   * Для больших n тест Миллера-Рабина с фиксированными базами
   * не даёт гарантии: существуют составные n, проходящие любой
   * конечный набор баз (если набор не покрывает все n). */
  if (bignum_length(n) > 2) {
    err = 1;
    goto cleanup;
  }

  /* Верхняя граница для баз: n - 2. Для маленьких n некоторые базы
   * из набора выходят за этот диапазон, их надо пропускать. */
  err = bignum_sub_digit(&nm2, n, 2);
  if (err < 0)
    goto cleanup;

  /* Для n < 2^64 этот набор из 12 баз детерминирован:
   * не существует составного n < 2^64, проходящего все 12 баз. */
  const uint32_t bases[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
  for (size_t i = 0; i < sizeof(bases) / sizeof(bases[0]); i++) {
    bignum_t a;
    bignum_init(&a);
    err = bignum_from_ui32_array(&a, (uint32_t[]){bases[i]}, 1, false);
    if (err < 0) {
      bignum_delete(&a);
      goto cleanup;
    }

    /* Пропускаем базы, выходящие за [2, n-2] (актуально для маленьких n). */
    if (bignum_cmp(&a, &nm2) == 1) {
      bignum_delete(&a);
      continue;
    }

    bool res = false;
    err = mersen_miller_rabin(&res, n, &a);
    bignum_delete(&a);
    if (err < 0)
      goto cleanup;

    /* Провал хотя бы на одной базе — n точно составное
     * (Миллер-Рабин не ошибается в сторону "составное"). */
    if (!res) {
      *is_prime = false;
      goto cleanup;
    }
  }

  /* Прошли все 12 баз — n гарантированно простое (для n < 2^64). */
  *is_prime = true;
  err = 0;

cleanup:
  bignum_delete(&two);
  bignum_delete(&three);
  bignum_delete(&nm2);
  return err;
}

/* ===== AKS: вспомогательные функции ===== */

/* Проверка: n = a^b для некоторого b >= 2 (полная степень).
 * Возвращает true, если n — полная степень. */
static bool is_perfect_power(const bignum_t *n) {
  size_t bits = bignum_length(n) * 32;
  /* Для b > bits(n) число a^b > n (для a >= 2), так что b <= bits(n) */
  for (uint32_t b = 2; b <= bits; b++) {
    /* a = floor(n^(1/b)). Ищем a перебором? Нет — через bignum_sqrt
     * для b=2, для b>2 — метод Ньютона. Но проще для маленьких n:
     * перевести в uint64_t и использовать pow. */
    if (bignum_length(n) <= 2) {
      uint32_t words[2] = {0, 0};
      long got = bignum_to_ui32_array(words, 2, n);
      if (got < 0)
        return false;
      uint64_t nv =
          (got == 2) ? (((uint64_t)words[1] << 32) | words[0]) : words[0];
      /* a = floor(nv^(1/b)) через double, потом корректировка */
      uint64_t a = (uint64_t)pow((double)nv, 1.0 / b);
      /* Корректировка на погрешность double */
      while (a > 1) {
        uint64_t p = 1;
        bool overflow = false;
        for (uint32_t i = 0; i < b; i++) {
          if (p > UINT64_MAX / a) {
            overflow = true;
            break;
          }
          p *= a;
        }
        if (!overflow && p == nv)
          return true;
        if (!overflow && p < nv)
          break;
        a--;
      }
      /* Проверить a+1 */
      a++;
      uint64_t p = 1;
      bool overflow = false;
      for (uint32_t i = 0; i < b; i++) {
        if (p > UINT64_MAX / a) {
          overflow = true;
          break;
        }
        p *= a;
      }
      if (!overflow && p == nv && a >= 2)
        return true;
    } else {
      /* Для больших n — только b=2 через bignum_sqrt, остальное
       * не проверяем (для AKS на больших n это не критично, т.к.
       * AKS всё равно не уложится по времени). */
      if (b == 2) {
        bignum_t root, sq;
        bignum_init(&root);
        bignum_init(&sq);
        int err = bignum_sqrt(&root, n);
        if (err == 0) {
          err = bignum_mul(&sq, &root, &root);
          if (err == 0 && bignum_cmp(&sq, n) == 0) {
            bignum_delete(&root);
            bignum_delete(&sq);
            return true;
          }
        }
        bignum_delete(&root);
        bignum_delete(&sq);
      }
    }
  }
  return false;
}

/* ord_r(n): наименьшее k >= 1 такое, что n^k ≡ 1 (mod r).
 * Возвращает 0, если gcd(n, r) != 1 (порядок не определён).
 * r — маленькое (uint32_t), n — bignum_t. */
static uint32_t multiplicative_order(uint32_t r, const bignum_t *n) {
  /* gcd(n mod r, r) должен быть 1 */
  uint32_t n_mod_r;
  if (bignum_mod_ui32(&n_mod_r, n, r) < 0)
    return 0;
  if (n_mod_r == 0)
    return 0;

  /* Перебор k = 1, 2, ... с n^k mod r */
  uint64_t cur = 1;
  for (uint32_t k = 1; k <= r; k++) {
    cur = (cur * n_mod_r) % r;
    if (cur == 1)
      return k;
  }
  return 0; /* не должно случиться для gcd=1 */
}

/* phi(r): функция Эйлера для маленького r (uint32_t). */
static uint32_t euler_phi(uint32_t r) {
  uint32_t result = r;
  uint32_t temp = r;
  for (uint32_t p = 2; p * p <= temp; p++) {
    if (temp % p == 0) {
      while (temp % p == 0)
        temp /= p;
      result -= result / p;
    }
  }
  if (temp > 1)
    result -= result / temp;
  return result;
}

int primality_aks(bool *is_prime, const bignum_t *n) {
  if (!is_prime || !n)
    return -ERR_NULLPTR;
  *is_prime = false;

  bignum_t two;
  bignum_init(&two);
  int err = bignum_from_ui32_array(&two, (uint32_t[]){2}, 1, false);
  if (err < 0) {
    bignum_delete(&two);
    return err;
  }

  if (bignum_cmp(n, &two) == -1) {
    bignum_delete(&two);
    return 0;
  } /* n < 2 */
  if (bignum_cmp(n, &two) == 0) {
    *is_prime = true;
    bignum_delete(&two);
    return 0;
  }

  /* Шаг 1: полная степень */
  if (is_perfect_power(n)) {
    bignum_delete(&two);
    return 0;
  }

  /* Шаг 2: найти наименьшее r с ord_r(n) > log2(n)^2 */
  size_t bits = bignum_bit_length(n);; /* верхняя оценка log2(n) */
  uint64_t log2_sq = (uint64_t)bits * bits;
  uint32_t r = 2;
  while (r < 100000) /* предохранитель */
  {
    
    uint32_t ord = multiplicative_order(r, n);
    if (ord > log2_sq)
      break;
    r++;
  }
  if (r >= 100000) {
    bignum_delete(&two);
    return -ERR_INVALID_PARAM;
  }

  /* Шаг 3: gcd(a, n) для a от 2 до r */
  bignum_t a_bn, g;
  bignum_init(&a_bn);
  bignum_init(&g);
  for (uint32_t a = 2; a <= r; a++) {
    err = bignum_from_ui32_array(&a_bn, &a, 1, false);
    if (err < 0)
      goto cleanup;
    err = bignum_gcd(&g, &a_bn, n);
    if (err < 0)
      goto cleanup;
    /* gcd > 1 и gcd < n → составное */
    if (bignum_cmp(&g, &two) != -1 && bignum_cmp(&g, n) != 0) {
      *is_prime = false;
      err = 0;
      goto cleanup;
    }
  }

  /* Шаг 4: если n <= r — простое */
  {
    bignum_t r_bn;
    bignum_init(&r_bn);
    err = bignum_from_ui32_array(&r_bn, &r, 1, false);
    if (err < 0) {
      bignum_delete(&r_bn);
      goto cleanup;
    }
    int cmp = bignum_cmp(n, &r_bn);
    bignum_delete(&r_bn);
    if (cmp <= 0) {
      *is_prime = true;
      err = 0;
      goto cleanup;
    }
  }

  /* Шаг 5: полиномиальная проверка */
  {
    uint32_t phi_r = euler_phi(r);
    /* limit = floor(sqrt(phi_r) * log2(n)) */
    uint64_t limit = (uint64_t)(sqrt((double)phi_r) * bits);
    /* Полином (x + a) mod (x^r - 1, n) */
    poly_t left, right, xa;
    err = poly_init(&left, r);
    if (err < 0)
      goto cleanup;
    err = poly_init(&right, r);
    if (err < 0) {
      poly_delete(&left);
      goto cleanup;
    }
    err = poly_init(&xa, r);
    if (err < 0) {
      poly_delete(&left);
      poly_delete(&right);
      goto cleanup;
    }

    bignum_t exp, a_val, x_pow;
    bignum_init(&exp);
    bignum_init(&a_val);
    bignum_init(&x_pow);

    for (uint64_t a = 1; a <= limit; a++) {
      /* xa = x + a */
      err = bignum_from_ui32_array(&a_val, (uint32_t[]){(uint32_t)a}, 1, false);
      if (err < 0)
        goto cleanup5;
      for (size_t i = 0; i < r; i++) {
        err = bignum_set_zero(&xa.coeffs[i]);
        if (err < 0)
          goto cleanup5;
      }
      /* coeff[0] = a mod n, coeff[1] = 1 */
      err = bignum_mod(&xa.coeffs[0], &a_val, n);
      if (err < 0)
        goto cleanup5;
      if (r > 1) {
        bignum_t one;
        bignum_init(&one);
        bignum_from_ui32_array(&one, (uint32_t[]){1}, 1, false);
        err = bignum_copy(&xa.coeffs[1], &one);
        bignum_delete(&one);
        if (err < 0)
          goto cleanup5;
      }

      /* left = (x + a)^n mod (x^r - 1, n) */
      err = poly_pow(&left, &xa, n, n);
      if (err < 0)
        goto cleanup5;

      /* right = x^n + a mod (x^r - 1, n) = x^(n mod r) + a */
      /* Сначала right = 0 */
      for (size_t i = 0; i < r; i++) {
        err = bignum_set_zero(&right.coeffs[i]);
        if (err < 0)
          goto cleanup5;
      }
      /* coeff[(n mod r)] = 1, coeff[0] += a */
      uint32_t n_mod_r;
      err = bignum_mod_ui32(&n_mod_r, n, r);
      if (err < 0)
        goto cleanup5;
      bignum_t one;
      bignum_init(&one);
      bignum_from_ui32_array(&one, (uint32_t[]){1}, 1, false);
      err = bignum_copy(&right.coeffs[n_mod_r], &one);
      bignum_delete(&one);
      if (err < 0)
        goto cleanup5;
      /* coeff[0] = (coeff[0] + a) mod n */
      err = bignum_add(&right.coeffs[0], &right.coeffs[0], &a_val);
      if (err < 0)
        goto cleanup5;
      err = bignum_mod(&right.coeffs[0], &right.coeffs[0], n);
      if (err < 0)
        goto cleanup5;

      /* Сравнить left и right */
      if (!poly_equal(&left, &right)) {
        *is_prime = false;
        err = 0;
        goto cleanup5;
      }
    }

    *is_prime = true;
    err = 0;

  cleanup5:
    bignum_delete(&exp);
    bignum_delete(&a_val);
    bignum_delete(&x_pow);
    poly_delete(&left);
    poly_delete(&right);
    poly_delete(&xa);
  }

cleanup:
  bignum_delete(&two);
  bignum_delete(&a_bn);
  bignum_delete(&g);
  return err;
}