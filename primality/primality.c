#include "primality.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
int mersen_miller_rabin(bool *is_prime, const bignum_t *val,
                        const bignum_t *base) {
  if (!val || !base) {
    return -ERR_NULLPTR;
  }
  bignum_t one, two, three, x, n1;
  bignum_init(&one);
  bignum_init(&two);
  bignum_init(&three);
  bignum_init(&x);
  bignum_init(&n1);
  int err = bignum_from_ui8_array(&one, (uint8_t[]){1}, 1, 0);
  if (err < 0) {
    goto cleanup;
  }
  err = bignum_from_ui8_array(&two, (uint8_t[]){2}, 1, 0);
  if (err < 0) {
    goto cleanup;
  }
  err = bignum_from_ui8_array(&three, (uint8_t[]){3}, 1, 0);
  if (err < 0) {
    *is_prime = false;
    goto cleanup;
  }
  err = bignum_sub_digit(&n1, val, 1);
  if (err < 0) {
    *is_prime = false;
    goto cleanup;
  }
  /* Проверки */
  if (bignum_cmp(val, &two) == -1) {
    *is_prime = false;
    goto cleanup;
  }
  if (bignum_cmp(val, &three) != 1) {
    *is_prime = true;
    goto cleanup;
  }
  if (bignum_cmp(base, &two) == -1) {
    *is_prime = false;
    err = -ERR_INVALID_PARAM;
    goto cleanup;
  }
  err = bignum_sub_digit(&x, val, 2);
  if (err < 0) {
    *is_prime = false;
    goto cleanup;
  }
  if (bignum_cmp(base, &x) == 1) {
    *is_prime = false;
    err = -ERR_INVALID_PARAM;
    goto cleanup;
  }
  /* Разложение N−1 = 2^s * d */

  err = bignum_copy(&x, &n1);
  if (err < 0) {
    *is_prime = false;
    goto cleanup;
  }
  size_t s = 0;
  while (bignum_is_even(&x)) {
    bignum_shift_right_bits(&x, 1);
    s++;
  }
  /* x = base^d mod N */
  err = bignum_pow_mod(&x, base, &x, val);
  if (err < 0) {
    *is_prime = false;
    goto cleanup;
  }
  /* Если x == 1 или x == N−1 — вероятно простое */
  if (bignum_cmp(&x, &one) == 0 || bignum_cmp(&x, &n1) == 0) {
    *is_prime = true;
    goto cleanup;
  }
  for (size_t i = 0; s > 0 && i < s - 1; i++) {
    err = bignum_pow_mod(&x, &x, &two, val);
    /* Если x = N−1 — вероятно простое */
    if (bignum_cmp(&x, &n1) == 0) {
      *is_prime = true;
      goto cleanup;
    }
    /* Если x = 1 — составное */
    if (bignum_cmp(&x, &one) == 0) {
      *is_prime = false;
      goto cleanup;
    }
  }
  *is_prime = false;
cleanup:
  bignum_delete(&one);
  bignum_delete(&two);
  bignum_delete(&three);
  bignum_delete(&x);
  bignum_delete(&n1);
  return err;
}

static int jacobi_operation(int *res, const bignum_t *val_1,
                            const bignum_t *val_2) {
  int err = 0;
  *res = 1;
  bignum_t val_1_copy, val_2_copy, one, tmp, three, five;
  bignum_init(&val_1_copy);
  bignum_init(&val_2_copy);
  bignum_init(&one);
  bignum_init(&three);
  bignum_init(&five);
  bignum_init(&tmp);
  err = bignum_copy(&val_1_copy, val_1);
  if (err < 0) {
    goto cleanup;
  }
  err = bignum_copy(&val_2_copy, val_2);
  if (err < 0) {
    goto cleanup;
  }
  err = bignum_from_ui8_array(&one, (uint8_t[]){1}, 1, 0);
  if (err < 0) {
    goto cleanup;
  }
  err = bignum_from_ui8_array(&three, (uint8_t[]){3}, 1, 0);
  if (err < 0) {
    goto cleanup;
  }
  err = bignum_from_ui8_array(&five, (uint8_t[]){5}, 1, 0);
  if (err < 0) {
    goto cleanup;
  }
  if (bignum_is_zero(&val_1_copy)) {
    if (bignum_cmp(&val_2_copy, &one) == 0) {
      err = bignum_copy(&val_1_copy, &one);
      if (err < 0) {
        goto cleanup;
      }
    }
  }
  while (!bignum_is_zero(&val_1_copy)) {
    while (bignum_is_even(&val_1_copy)) {
      bignum_shift_right_bits(&val_1_copy, 1);
      uint32_t mod8 = 0;
      err = bignum_mod_ui32(&mod8, &val_2_copy, 8);
      if (err < 0) {
        goto cleanup;
      }
      if (mod8 == 3 || mod8 == 5) {
        *res = -(*res);
      }
    }
    err = bignum_copy(&tmp, &val_1_copy);
    if (err < 0) {
      goto cleanup;
    }
    err = bignum_copy(&val_1_copy, &val_2_copy);
    if (err < 0) {
      goto cleanup;
    }
    err = bignum_copy(&val_2_copy, &tmp);
    if (err < 0) {
      goto cleanup;
    }
    uint32_t mod4 = 0;
    err = bignum_mod_ui32(&mod4, &val_2_copy, 4);
    if (err < 0) {
      goto cleanup;
    }
    if (mod4 == 3) {
      err = bignum_mod_ui32(&mod4, &val_1_copy, 4);
      if (err < 0) {
        goto cleanup;
      }
      if (mod4 == 3) {
        *res = -(*res);
      }
    }
    err = bignum_mod(&val_1_copy, &val_1_copy, &val_2_copy);
    if (err < 0) {
      goto cleanup;
    }
  }
  if (bignum_cmp(&val_2_copy, &one) != 0) {
    *res = 0;
  }

cleanup:
  bignum_delete(&val_1_copy);
  bignum_delete(&val_2_copy);
  bignum_delete(&one);
  bignum_delete(&three);
  bignum_delete(&five);
  bignum_delete(&tmp);
  return err;
}

/* Ищет D такое, что Jacobi(D, N) == -1.
 * P = 1 всегда, Q = (1 - D) / 4.
 * Возвращает 0 при успехе, -ERR_* при ошибке,
 * 1 если Jacobi(D, N) == 0 (N делится на D — составное).
 * Если |D| > 1000 — -ERR_INVALID_PARAM (не нашли). */
static int get_jacobi_pair(int32_t *D_out, int32_t *Q_out, const bignum_t *N) {
  int32_t D = 5;
  int sign = 1;

  while (labs(D) <= 1000) {
    /* Привести D к положительному [0, N-1] для jacobi */
    bignum_t d_pos;
    bignum_init(&d_pos);

    int err;
    if (D >= 0) {
      err = bignum_from_ui32_array(&d_pos, (uint32_t[]){(uint32_t)D}, 1, false);
    } else {
      /* d_pos = N - |D| (для |D| < N) */
      err = bignum_sub_digit(&d_pos, N, (uint32_t)(-D));
    }
    if (err < 0) {
      bignum_delete(&d_pos);
      return err;
    }

    int jac = 0;
    err = jacobi_operation(&jac, &d_pos, N);
    bignum_delete(&d_pos);
    if (err < 0)
      return err;

    if (jac == -1) {
      *D_out = D;
      *Q_out = (1 - D) / 4;
      return 0;
    }
    if (jac == 0) {
      /* N делится на D (или D и N не взаимно простые) */
      return 1;
    }

    /* Следующее D: 5, -7, 9, -11, ... */
    sign = -sign;
    D = sign * (labs(D) + 2);
  }

  return -ERR_INVALID_PARAM;
}

/* Деление на 2 по модулю N (N нечётное). x = x / 2 mod N. */
static int div2_mod(bignum_t *x, const bignum_t *N) {
  int err;
  if (bignum_is_even(x)) {
    bignum_shift_right_bits(x, 1);
    return 0;
  }
  /* x нечётное: (x + N) / 2 */
  err = bignum_add(x, x, N);
  if (err < 0)
    return err;
  bignum_shift_right_bits(x, 1);
  return 0;
}

/* Вычисляет U_d, V_d, Q^d mod N для последовательности Люка с P, Q.
 * P, Q — bignum_t (положительные, < N), D — bignum_t (D = P^2 - 4Q mod N),
 * d — bignum_t (показатель), N — bignum_t (модуль, нечётный).
 * U, V, Qk — выходные (инициализированы снаружи). */
static int lucas_sequence(bignum_t *U, bignum_t *V, bignum_t *Qk,
                          const bignum_t *P, const bignum_t *Q,
                          const bignum_t *D, const bignum_t *d,
                          const bignum_t *N) {
  int err = 0;
  bignum_t U2, V2, Qk2, tmp;
  bignum_init(&U2);
  bignum_init(&V2);
  bignum_init(&Qk2);
  bignum_init(&tmp);

  /* k = 1: U = 1, V = P, Qk = Q */
  uint32_t one = 1;
  err = bignum_from_ui32_array(U, &one, 1, false);
  if (err < 0)
    goto cleanup;
  err = bignum_copy(V, P);
  if (err < 0)
    goto cleanup;
  err = bignum_copy(Qk, Q);
  if (err < 0)
    goto cleanup;

  /* Идём по битам d с старшего (пропуская самый старший, он всегда 1) */
  size_t bits = bignum_length(d) * 32;
  /* Найти реальную длину в битах */
  while (bits > 1 && !bignum_bit(d, bits - 1))
    bits--;

  for (size_t i = bits - 1; i > 0; i--) {
    /* Удвоение: k -> 2k */
    /* U2 = U * V */
    err = bignum_mul(&U2, U, V);
    if (err < 0)
      goto cleanup;
    /* V2 = V*V - 2*Qk */
    err = bignum_mul(&V2, V, V);
    if (err < 0)
      goto cleanup;
    err = bignum_add(&tmp, Qk, Qk); /* 2*Qk */
    if (err < 0)
      goto cleanup;
    err = bignum_sub(&V2, &V2, &tmp);
    if (err < 0)
      goto cleanup;
    err = bignum_mod(&V2, &V2, N);
    if (err < 0)
      goto cleanup;
    /* Qk2 = Qk * Qk mod N */
    err = bignum_mul(&Qk2, Qk, Qk);
    if (err < 0)
      goto cleanup;
    err = bignum_mod(&Qk2, &Qk2, N);
    if (err < 0)
      goto cleanup;

    /* U2, V2, Qk2 — это U_{2k}, V_{2k}, Q^{2k} */

    if (bignum_bit(d, i - 1)) {
      /* Добавление: 2k -> 2k+1 */
      /* U_new = (P*U2 + V2) / 2 mod N */
      bignum_t U_new, V_new;
      bignum_init(&U_new);
      bignum_init(&V_new);

      err = bignum_mul(&U_new, P, &U2);
      if (err < 0) {
        bignum_delete(&U_new);
        bignum_delete(&V_new);
        goto cleanup;
      }
      err = bignum_add(&U_new, &U_new, &V2);
      if (err < 0) {
        bignum_delete(&U_new);
        bignum_delete(&V_new);
        goto cleanup;
      }
      err = bignum_mod(&U_new, &U_new, N);
      if (err < 0) {
        bignum_delete(&U_new);
        bignum_delete(&V_new);
        goto cleanup;
      }
      err = div2_mod(&U_new, N);
      if (err < 0) {
        bignum_delete(&U_new);
        bignum_delete(&V_new);
        goto cleanup;
      }

      /* V_new = (D*U2 + P*V2) / 2 mod N */
      err = bignum_mul(&V_new, D, &U2);
      if (err < 0) {
        bignum_delete(&U_new);
        bignum_delete(&V_new);
        goto cleanup;
      }
      err = bignum_mul(&tmp, P, &V2);
      if (err < 0) {
        bignum_delete(&U_new);
        bignum_delete(&V_new);
        goto cleanup;
      }
      err = bignum_add(&V_new, &V_new, &tmp);
      if (err < 0) {
        bignum_delete(&U_new);
        bignum_delete(&V_new);
        goto cleanup;
      }
      err = bignum_mod(&V_new, &V_new, N);
      if (err < 0) {
        bignum_delete(&U_new);
        bignum_delete(&V_new);
        goto cleanup;
      }
      err = div2_mod(&V_new, N);
      if (err < 0) {
        bignum_delete(&U_new);
        bignum_delete(&V_new);
        goto cleanup;
      }

      /* Qk_new = Qk2 * Q mod N */
      err = bignum_mul(&Qk2, &Qk2, Q);
      if (err < 0) {
        bignum_delete(&U_new);
        bignum_delete(&V_new);
        goto cleanup;
      }
      err = bignum_mod(&Qk2, &Qk2, N);
      if (err < 0) {
        bignum_delete(&U_new);
        bignum_delete(&V_new);
        goto cleanup;
      }

      err = bignum_copy(&U2, &U_new);
      if (err < 0) {
        bignum_delete(&U_new);
        bignum_delete(&V_new);
        goto cleanup;
      }
      err = bignum_copy(&V2, &V_new);
      if (err < 0) {
        bignum_delete(&U_new);
        bignum_delete(&V_new);
        goto cleanup;
      }
      bignum_delete(&U_new);
      bignum_delete(&V_new);
    }

    /* Присвоить U = U2, V = V2, Qk = Qk2 */
    err = bignum_copy(U, &U2);
    if (err < 0)
      goto cleanup;
    err = bignum_copy(V, &V2);
    if (err < 0)
      goto cleanup;
    err = bignum_copy(Qk, &Qk2);
    if (err < 0)
      goto cleanup;
  }

cleanup:
  bignum_delete(&U2);
  bignum_delete(&V2);
  bignum_delete(&Qk2);
  bignum_delete(&tmp);
  return err;
}
int primality_lucas_strong(bool *is_prime, const bignum_t *val) {
  if (!is_prime || !val)
    return -ERR_NULLPTR;
  *is_prime = false;

  /* val должно быть нечётным, > 3 */
  bignum_t one, two, three;
  bignum_init(&one);
  bignum_init(&two);
  bignum_init(&three);
  bignum_from_ui32_array(&one, (uint32_t[]){1}, 1, false);
  bignum_from_ui32_array(&two, (uint32_t[]){2}, 1, false);
  bignum_from_ui32_array(&three, (uint32_t[]){3}, 1, false);

  int err = 0;
  if (bignum_cmp(val, &two) == -1)
    goto cleanup; // val < 2
  if (bignum_cmp(val, &two) == 0 || bignum_cmp(val, &three) == 0) {
    *is_prime = true;
    goto cleanup;
  }
  if (bignum_is_even(val))
    goto cleanup; // чётное > 2 — составное

  /* Проверка полного квадрата: если val = k^2 — составное */
  bignum_t root;
  bignum_init(&root);
  err = bignum_sqrt(&root, val);
  if (err < 0) {
    bignum_delete(&root);
    goto cleanup;
  }
  bignum_t root_sq;
  bignum_init(&root_sq);
  err = bignum_mul(&root_sq, &root, &root);
  bignum_delete(&root);
  if (err < 0) {
    bignum_delete(&root_sq);
    goto cleanup;
  }
  if (bignum_cmp(&root_sq, val) == 0) {
    bignum_delete(&root_sq);
    goto cleanup;
  } // полный квадрат
  bignum_delete(&root_sq);

  /* Выбор D, Q */
  int32_t D_int, Q_int;
  err = get_jacobi_pair(&D_int, &Q_int, val);
  if (err < 0)
    goto cleanup;
  if (err == 1)
    goto cleanup; // Jacobi == 0, val составное

  /* D, Q, P=1 — в bignum_t (положительные mod val) */
  bignum_t P_bn, Q_bn, D_bn;
  bignum_init(&P_bn);
  bignum_init(&Q_bn);
  bignum_init(&D_bn);

  bignum_from_ui32_array(&P_bn, (uint32_t[]){1}, 1, false);

  if (Q_int < 0)
    err = bignum_sub_digit(&Q_bn, val, (uint32_t)(-Q_int));
  else
    err =
        bignum_from_ui32_array(&Q_bn, (uint32_t[]){(uint32_t)Q_int}, 1, false);
  if (err < 0)
    goto cleanup2;

  if (D_int < 0)
    err = bignum_sub_digit(&D_bn, val, (uint32_t)(-D_int));
  else
    err =
        bignum_from_ui32_array(&D_bn, (uint32_t[]){(uint32_t)D_int}, 1, false);
  if (err < 0)
    goto cleanup2;

  /* val + 1 = 2^s * d */
  bignum_t nm1, d;
  bignum_init(&nm1);
  bignum_init(&d);
  err = bignum_add_digit(&nm1, val, 1); // val + 1
  if (err < 0)
    goto cleanup3;
  err = bignum_copy(&d, &nm1);
  if (err < 0)
    goto cleanup3;
  size_t s = 0;
  while (bignum_is_even(&d)) {
    bignum_shift_right_bits(&d, 1);
    s++;
  }

  /* U_d, V_d, Q^d mod val */
  bignum_t U, V, Qk;
  bignum_init(&U);
  bignum_init(&V);
  bignum_init(&Qk);
  err = lucas_sequence(&U, &V, &Qk, &P_bn, &Q_bn, &D_bn, &d, val);
  if (err < 0)
    goto cleanup4;

  /* Проверка: U_d == 0 или V_d == 0 → вероятно простое */
  if (bignum_is_zero(&U) || bignum_is_zero(&V)) {
    *is_prime = true;
    goto cleanup4;
  }

  /* s-1 раз: V = V^2 - 2*Qk mod val */
  for (size_t i = 0; i < s - 1; i++) {
    err = bignum_mul(&V, &V, &V);
    if (err < 0)
      goto cleanup4;
    bignum_add(&nm1, &Qk, &Qk); // 2*Qk
    err = bignum_sub(&V, &V, &nm1);
    if (err < 0)
      goto cleanup4;
    err = bignum_mod(&V, &V, val);
    if (err < 0)
      goto cleanup4;
    if (bignum_is_zero(&V)) {
      *is_prime = true;
      goto cleanup4;
    }

    /* Qk = Qk^2 mod val */
    err = bignum_mul(&Qk, &Qk, &Qk);
    if (err < 0)
      goto cleanup4;
    err = bignum_mod(&Qk, &Qk, val);
    if (err < 0)
      goto cleanup4;
  }

cleanup4:
  bignum_delete(&U);
  bignum_delete(&V);
  bignum_delete(&Qk);
cleanup3:
  bignum_delete(&nm1);
  bignum_delete(&d);
cleanup2:
  bignum_delete(&P_bn);
  bignum_delete(&Q_bn);
  bignum_delete(&D_bn);
cleanup:
  bignum_delete(&one);
  bignum_delete(&two);
  bignum_delete(&three);
  return err;
}

int primality_bpsw(bool *is_prime, const bignum_t *val) {
  if (!is_prime || !val)
    return -ERR_NULLPTR;

  int err = 0;
  bignum_t two, three;
  bignum_init(&two);
  bignum_init(&three);

  err = bignum_from_ui32_array(&two, (uint32_t[]){2}, 1, false);
  if (err < 0)
    goto cleanup;
  err = bignum_from_ui32_array(&three, (uint32_t[]){3}, 1, false);
  if (err < 0)
    goto cleanup;

  /* val < 2 → не простое */
  if (bignum_cmp(val, &two) == -1) {
    *is_prime = false;
    goto cleanup;
  }
  /* val == 2 или 3 → простое */
  if (bignum_cmp(val, &two) == 0 || bignum_cmp(val, &three) == 0) {
    *is_prime = true;
    goto cleanup;
  }

  /* Миллер-Рабин с базой 2 */
  bool mr = false;
  err = mersen_miller_rabin(&mr, val, &two);
  if (err < 0)
    goto cleanup;
  // #ifdef PRIMALITY_DEBUG
  //   fprintf(stderr, "  [bpsw] MR(2): %s\n", mr ? "prime" : "composite");
  // #endif
  if (!mr) {
    *is_prime = false;
    goto cleanup;
  }

  /* Сильный Люк */
  bool lucas = false;
  err = primality_lucas_strong(&lucas, val);
  if (err < 0)
    goto cleanup;
  // #ifdef PRIMALITY_DEBUG
  //   fprintf(stderr, "  [bpsw] Lucas: %s\n", lucas ? "prime" : "composite");
  // #endif

  *is_prime = lucas;

cleanup:
  bignum_delete(&two);
  bignum_delete(&three);
  return err;
}

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