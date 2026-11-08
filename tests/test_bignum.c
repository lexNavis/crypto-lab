#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gmp.h>
#include "../bignum/bignum.h"
#include "../common/config.h"
#include "args.h"

/* Вспомогательная: сверить bignum с mpz. Возвращает 0, если равны, иначе -1. */
static int check_bignum_eq_gmp(const bignum_t *b, const mpz_t g, const char *label)
{
  mpz_t tmp;
  mpz_init(tmp);
  bignum_to_gmp(tmp, b);
  if (mpz_cmp(tmp, g) != 0)
  {
    fprintf(stderr, "FAIL %s\n  bignum: ", label);
    bignum_print_dec(b);
    fprintf(stderr, "\n  gmp:    ");
    gmp_fprintf(stderr, "%Zd\n", g);
    mpz_clear(tmp);
    return -1;
  }
  mpz_clear(tmp);
  return 0;
}

/* Общий прогон одного кейса mul с указанным режимом алиасинга.
 * mode: 0 = res == val_1, 1 = res == val_2, 2 = все три равны.
 * Для mode == 2 a_hex и b_hex должны совпадать. */
static int test_mul_alias(const char *a_hex, const char *b_hex, int mode)
{
  bignum_t A, B, R;
  bignum_init(&A);
  bignum_init(&B);
  bignum_init(&R);

  int rc = 0;
  int err = bignum_from_hex_string(&A, a_hex, false);
  if (err < 0)
  {
    rc = -1;
    goto cleanup;
  }
  err = bignum_from_hex_string(&B, b_hex, false);
  if (err < 0)
  {
    rc = -1;
    goto cleanup;
  }

  mpz_t ga, gb, gr;
  mpz_init(ga);
  mpz_init(gb);
  mpz_init(gr);
  bignum_to_gmp(ga, &A);
  bignum_to_gmp(gb, &B);
  mpz_mul(gr, ga, gb);

  if (mode == 0)
  {
    bignum_copy(&R, &A);
    err = bignum_mul(&R, &R, &B);
    if (err < 0)
    {
      rc = -1;
      goto gmp_cleanup;
    }
    if (check_bignum_eq_gmp(&R, gr, "mul res==val_1") < 0)
    {
      rc = -1;
      goto gmp_cleanup;
    }
  }
  else if (mode == 1)
  {
    bignum_copy(&R, &B);
    err = bignum_mul(&R, &A, &R);
    if (err < 0)
    {
      rc = -1;
      goto gmp_cleanup;
    }
    if (check_bignum_eq_gmp(&R, gr, "mul res==val_2") < 0)
    {
      rc = -1;
      goto gmp_cleanup;
    }
  }
  else
  {
    err = bignum_mul(&A, &A, &A);
    if (err < 0)
    {
      rc = -1;
      goto gmp_cleanup;
    }
    /* эталон: a * a, gr уже = a * b, b == a */
    if (check_bignum_eq_gmp(&A, gr, "mul res==val_1==val_2") < 0)
    {
      rc = -1;
      goto gmp_cleanup;
    }
  }

gmp_cleanup:
  mpz_clear(ga);
  mpz_clear(gb);
  mpz_clear(gr);
cleanup:
  bignum_delete(&A);
  bignum_delete(&B);
  bignum_delete(&R);
  return rc;
}

int run_bignum_mul_alias_tests(void)
{
  int rc = 0;

  const char *A = "ffffffffffffffffffffffffffffffff"; // 128 бит, все 1
  const char *B = "1";
  const char *C = "80000000000000000000000000000000"; // старший бит
  const char *D = "ffffffff";                         // одно слово
  const char *E = "100000000";                        // слово + 1
  const char *F = "123456789abcdef0fedcba9876543210"; // произвольное

  /* mode 0: res == val_1 */
  rc |= test_mul_alias(A, B, 0); // длинное * 1
  rc |= test_mul_alias(A, D, 0); // длинное * короткое
  rc |= test_mul_alias(D, E, 0); // короткое * короткое, перенос
  rc |= test_mul_alias(C, C, 0); // старший бит сам на себя
  rc |= test_mul_alias(F, F, 0); // произвольное

  /* mode 1: res == val_2 */
  rc |= test_mul_alias(A, B, 1);
  rc |= test_mul_alias(A, D, 1);
  rc |= test_mul_alias(D, E, 1);
  rc |= test_mul_alias(C, C, 1);
  rc |= test_mul_alias(F, F, 1);

  /* mode 2: все три равны */
  rc |= test_mul_alias(A, A, 2);
  rc |= test_mul_alias(D, D, 2);
  rc |= test_mul_alias(C, C, 2);
  rc |= test_mul_alias(F, F, 2);

  if (rc == 0)
    fprintf(stderr, "ALL MUL ALIAS TESTS PASSED\n");
  else
    fprintf(stderr, "MUL ALIAS TESTS FAILED\n");

  return rc;
}

/* Общий прогон одного кейса divmod с указанным режимом алиасинга.
 * mode:
 *   0 = quot == val_1
 *   1 = rem == val_1
 *   2 = quot == val_2
 *   3 = rem == val_2
 *   4 = quot == val_1, rem == val_2
 *   5 = quot == val_2, rem == val_1
 *   6 = quot == rem (должно вернуть ошибку)
 */
static int test_divmod_alias(const char *a_hex, const char *b_hex, int mode)
{
  bignum_t A, B, Q, R;
  bignum_init(&A);
  bignum_init(&B);
  bignum_init(&Q);
  bignum_init(&R);

  int rc = 0;
  int err = bignum_from_hex_string(&A, a_hex, false);
  if (err < 0) { rc = -1; goto cleanup; }
  err = bignum_from_hex_string(&B, b_hex, false);
  if (err < 0) { rc = -1; goto cleanup; }

  /* GMP-эталон */
  mpz_t ga, gb, gq, gr;
  mpz_init(ga); mpz_init(gb); mpz_init(gq); mpz_init(gr);
  bignum_to_gmp(ga, &A);
  bignum_to_gmp(gb, &B);

  if (mode == 6)
  {
    /* quot == rem — должно быть ошибкой */
    err = bignum_divmod(&Q, &Q, &A, &B);
    if (err >= 0)
    {
      fprintf(stderr, "FAIL divmod quot==rem: expected error, got %d\n", err);
      rc = -1;
    }
    goto gmp_cleanup;
  }

  mpz_tdiv_qr(gq, gr, ga, gb);

  if (mode == 0)
  {
    bignum_copy(&Q, &A);
    err = bignum_divmod(&Q, &R, &Q, &B);
    if (err < 0) { rc = -1; goto gmp_cleanup; }
    if (check_bignum_eq_gmp(&Q, gq, "divmod quot==val_1 (q)") < 0) { rc = -1; goto gmp_cleanup; }
    if (check_bignum_eq_gmp(&R, gr, "divmod quot==val_1 (r)") < 0) { rc = -1; goto gmp_cleanup; }
  }
  else if (mode == 1)
  {
    bignum_copy(&R, &A);
    err = bignum_divmod(&Q, &R, &R, &B);
    if (err < 0) { rc = -1; goto gmp_cleanup; }
    if (check_bignum_eq_gmp(&Q, gq, "divmod rem==val_1 (q)") < 0) { rc = -1; goto gmp_cleanup; }
    if (check_bignum_eq_gmp(&R, gr, "divmod rem==val_1 (r)") < 0) { rc = -1; goto gmp_cleanup; }
  }
  else if (mode == 2)
  {
    bignum_copy(&Q, &B);
    err = bignum_divmod(&Q, &R, &A, &Q);
    if (err < 0) { rc = -1; goto gmp_cleanup; }
    if (check_bignum_eq_gmp(&Q, gq, "divmod quot==val_2 (q)") < 0) { rc = -1; goto gmp_cleanup; }
    if (check_bignum_eq_gmp(&R, gr, "divmod quot==val_2 (r)") < 0) { rc = -1; goto gmp_cleanup; }
  }
  else if (mode == 3)
  {
    bignum_copy(&R, &B);
    err = bignum_divmod(&Q, &R, &A, &R);
    if (err < 0) { rc = -1; goto gmp_cleanup; }
    if (check_bignum_eq_gmp(&Q, gq, "divmod rem==val_2 (q)") < 0) { rc = -1; goto gmp_cleanup; }
    if (check_bignum_eq_gmp(&R, gr, "divmod rem==val_2 (r)") < 0) { rc = -1; goto gmp_cleanup; }
  }
  else if (mode == 4)
  {
    /* quot == val_1, rem == val_2: A и B будут затёрты, но они больше не нужны */
    err = bignum_divmod(&A, &B, &A, &B);
    if (err < 0) { rc = -1; goto gmp_cleanup; }
    if (check_bignum_eq_gmp(&A, gq, "divmod q==v1,r==v2 (q)") < 0) { rc = -1; goto gmp_cleanup; }
    if (check_bignum_eq_gmp(&B, gr, "divmod q==v1,r==v2 (r)") < 0) { rc = -1; goto gmp_cleanup; }
  }
  else if (mode == 5)
  {
    /* quot == val_2, rem == val_1 */
    err = bignum_divmod(&B, &A, &A, &B);
    if (err < 0) { rc = -1; goto gmp_cleanup; }
    if (check_bignum_eq_gmp(&B, gq, "divmod q==v2,r==v1 (q)") < 0) { rc = -1; goto gmp_cleanup; }
    if (check_bignum_eq_gmp(&A, gr, "divmod q==v2,r==v1 (r)") < 0) { rc = -1; goto gmp_cleanup; }
  }

gmp_cleanup:
  mpz_clear(ga); mpz_clear(gb); mpz_clear(gq); mpz_clear(gr);
cleanup:
  bignum_delete(&A);
  bignum_delete(&B);
  bignum_delete(&Q);
  bignum_delete(&R);
  return rc;
}

int run_bignum_divmod_alias_tests(void)
{
  int rc = 0;

  const char *A = "ffffffffffffffffffffffffffffffff";  /* 128 бит */
  const char *B = "1";
  const char *C = "80000000000000000000000000000000";
  const char *D = "ffffffff";
  const char *E = "100000000";
  const char *F = "123456789abcdef0fedcba9876543210";
  const char *G = "123456789abcdef";                     /* меньше F */
  const char *H = "123456789abcdef0fedcba9876543210";    /* == F */

  /* Основной путь, все режимы алиасинга */
  rc |= test_divmod_alias(F, G, 0);  /* quot==v1 */
  rc |= test_divmod_alias(F, G, 1);  /* rem==v1 */
  rc |= test_divmod_alias(F, G, 2);  /* quot==v2 */
  rc |= test_divmod_alias(F, G, 3);  /* rem==v2 */
  rc |= test_divmod_alias(F, G, 4);  /* q==v1, r==v2 */
  rc |= test_divmod_alias(F, G, 5);  /* q==v2, r==v1 */

  /* Тривиальный: |a| < |b| */
  rc |= test_divmod_alias(G, F, 0);
  rc |= test_divmod_alias(G, F, 1);
  rc |= test_divmod_alias(G, F, 2);
  rc |= test_divmod_alias(G, F, 3);
  rc |= test_divmod_alias(G, F, 4);
  rc |= test_divmod_alias(G, F, 5);

  /* Тривиальный: |a| == |b| */
  rc |= test_divmod_alias(F, H, 0);
  rc |= test_divmod_alias(F, H, 1);
  rc |= test_divmod_alias(F, H, 2);
  rc |= test_divmod_alias(F, H, 3);
  rc |= test_divmod_alias(F, H, 4);
  rc |= test_divmod_alias(F, H, 5);

  /* Деление на 1, деление одинаковых, деление степени двойки */
  rc |= test_divmod_alias(A, B, 0);
  rc |= test_divmod_alias(A, B, 1);
  rc |= test_divmod_alias(D, E, 0);
  rc |= test_divmod_alias(C, C, 0);
  rc |= test_divmod_alias(C, C, 4);

  /* quot == rem — ошибка */
  rc |= test_divmod_alias(F, G, 6);

  if (rc == 0)
    fprintf(stderr, "ALL DIVMOD ALIAS TESTS PASSED\n");
  else
    fprintf(stderr, "DIVMOD ALIAS TESTS FAILED\n");

  return rc;
}

int test_sqrt_small(void)
{
  int rc = 0;
  const uint64_t cases[] = {
    0, 1, 2, 3, 4, 5, 8, 9, 15, 16, 17, 24, 25, 26,
    99, 100, 101, 255, 256, 257,
    65535, 65536, 65537,
    4294967295ULL, 4294967296ULL, 4294967297ULL,
    18446744073709551615ULL  /* 2^64 - 1 */
  };
  for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
    bignum_t n, r;
    bignum_init(&n);
    bignum_init(&r);
    uint32_t words[2];
    uint64_t v = cases[i];
    words[0] = (uint32_t)v;
    words[1] = (uint32_t)(v >> 32);
    size_t len = (words[1] != 0) ? 2 : 1;
    bignum_from_ui32_array(&n, words, len, false);
    int err = bignum_sqrt(&r, &n);
    if (err < 0) {
      fprintf(stderr, "FAIL sqrt(%llx): err=%d\n", (unsigned long long)v, err);
      rc = -1;
    } else {
      uint32_t got_words[2] = {0, 0};
      long got_len = bignum_to_ui32_array(got_words, 2, &r);
      if (got_len < 0) {
        fprintf(stderr, "FAIL sqrt(%llx): to_ui32_array err=%ld\n", (unsigned long long)v, got_len);
        rc = -1;
      } else {
        uint64_t got = (got_len == 2)
          ? (((uint64_t)got_words[1] << 32) | got_words[0])
          : got_words[0];
        __uint128_t expected = (__uint128_t)sqrt((double)v);
        __uint128_t sq = (__uint128_t)(expected + 1) * (expected + 1);
        while (sq <= v) { expected++; sq = (__uint128_t)(expected + 1) * (expected + 1); }
        while ((__uint128_t)expected * expected > v) expected--;
        if (got != expected) {
          fprintf(stderr, "FAIL sqrt(%llx): got %llx, expected %llx\n",
                  (unsigned long long)v,
                  (unsigned long long)got,
                  (unsigned long long)expected);
          rc = -1;
        }
      }
    }
    bignum_delete(&n);
    bignum_delete(&r);
  }
  if (rc == 0) fprintf(stderr, "ALL SQRT TESTS PASSED\n");
  else fprintf(stderr, "SQRT TESTS FAILED\n");
  return rc;
}
/* Общий прогон одного кейса add с указанным режимом алиасинга.
 * mode: 0 = res == val_1, 1 = res == val_2, 2 = все три равны.
 * Для mode == 2 a_hex и b_hex должны совпадать. */
static int test_add_alias(const char *a_hex, const char *b_hex, int mode)
{
  bignum_t A, B, R;
  bignum_init(&A);
  bignum_init(&B);
  bignum_init(&R);

  int rc = 0;
  int err = bignum_from_hex_string(&A, a_hex, false);
  if (err < 0)
  {
    rc = -1;
    goto cleanup;
  }
  err = bignum_from_hex_string(&B, b_hex, false);
  if (err < 0)
  {
    rc = -1;
    goto cleanup;
  }

  /* GMP-эталон */
  mpz_t ga, gb, gr;
  mpz_init(ga);
  mpz_init(gb);
  mpz_init(gr);
  bignum_to_gmp(ga, &A);
  bignum_to_gmp(gb, &B);
  mpz_add(gr, ga, gb);

  /* Твой вызов с алиасингом */
  if (mode == 0)
  {
    bignum_copy(&R, &A);
    err = bignum_add(&R, &R, &B);
    if (err < 0)
    {
      rc = -1;
      goto gmp_cleanup;
    }
    if (check_bignum_eq_gmp(&R, gr, "add res==val_1") < 0)
    {
      rc = -1;
      goto gmp_cleanup;
    }
  }
  else if (mode == 1)
  {
    bignum_copy(&R, &B);
    err = bignum_add(&R, &A, &R);
    if (err < 0)
    {
      rc = -1;
      goto gmp_cleanup;
    }
    if (check_bignum_eq_gmp(&R, gr, "add res==val_2") < 0)
    {
      rc = -1;
      goto gmp_cleanup;
    }
  }
  else
  {
    err = bignum_add(&A, &A, &A);
    if (err < 0)
    {
      rc = -1;
      goto gmp_cleanup;
    }
    /* эталон: a + a, но у нас gr = a + b, а b == a в mode==2,
     * так что gr валиден */
    if (check_bignum_eq_gmp(&A, gr, "add res==val_1==val_2") < 0)
    {
      rc = -1;
      goto gmp_cleanup;
    }
  }

gmp_cleanup:
  mpz_clear(ga);
  mpz_clear(gb);
  mpz_clear(gr);
cleanup:
  bignum_delete(&A);
  bignum_delete(&B);
  bignum_delete(&R);
  return rc;
}

/* То же самое для sub. */
static int test_sub_alias(const char *a_hex, const char *b_hex, int mode)
{
  bignum_t A, B, R;
  bignum_init(&A);
  bignum_init(&B);
  bignum_init(&R);

  int rc = 0;
  int err = bignum_from_hex_string(&A, a_hex, false);
  if (err < 0)
  {
    rc = -1;
    goto cleanup;
  }
  err = bignum_from_hex_string(&B, b_hex, false);
  if (err < 0)
  {
    rc = -1;
    goto cleanup;
  }

  mpz_t ga, gb, gr;
  mpz_init(ga);
  mpz_init(gb);
  mpz_init(gr);
  bignum_to_gmp(ga, &A);
  bignum_to_gmp(gb, &B);
  mpz_sub(gr, ga, gb);

  if (mode == 0)
  {
    bignum_copy(&R, &A);
    err = bignum_sub(&R, &R, &B);
    if (err < 0)
    {
      rc = -1;
      goto gmp_cleanup;
    }
    if (check_bignum_eq_gmp(&R, gr, "sub res==val_1") < 0)
    {
      rc = -1;
      goto gmp_cleanup;
    }
  }
  else if (mode == 1)
  {
    bignum_copy(&R, &B);
    err = bignum_sub(&R, &A, &R);
    if (err < 0)
    {
      rc = -1;
      goto gmp_cleanup;
    }
    if (check_bignum_eq_gmp(&R, gr, "sub res==val_2") < 0)
    {
      rc = -1;
      goto gmp_cleanup;
    }
  }
  else
  {
    err = bignum_sub(&A, &A, &A);
    if (err < 0)
    {
      rc = -1;
      goto gmp_cleanup;
    }
    /* эталон: a - a = 0 */
    mpz_set_ui(gr, 0);
    if (check_bignum_eq_gmp(&A, gr, "sub res==val_1==val_2") < 0)
    {
      rc = -1;
      goto gmp_cleanup;
    }
  }

gmp_cleanup:
  mpz_clear(ga);
  mpz_clear(gb);
  mpz_clear(gr);
cleanup:
  bignum_delete(&A);
  bignum_delete(&B);
  bignum_delete(&R);
  return rc;
}

int run_bignum_alias_tests(void)
{
  int rc = 0;

  /* Числа для тестов */
  const char *A = "ffffffffffffffffffffffffffffffff";
  const char *B = "1";
  const char *C = "80000000000000000000000000000000";
  const char *D = "ffffffff";
  const char *E = "100000000";

  /* add, алиасинг res == val_1 */
  rc |= test_add_alias(A, B, 0);
  rc |= test_add_alias(D, E, 0); /* перенос в новое слово */
  rc |= test_add_alias(C, B, 0);

  /* add, алиасинг res == val_2 */
  rc |= test_add_alias(A, B, 1);
  rc |= test_add_alias(D, E, 1);
  rc |= test_add_alias(C, B, 1);

  /* add, все три равны */
  rc |= test_add_alias(A, A, 2);
  rc |= test_add_alias(D, D, 2);
  rc |= test_add_alias(C, C, 2);

  /* sub, алиасинг res == val_1 */
  rc |= test_sub_alias(A, B, 0); /* |a| > |b| */
  rc |= test_sub_alias(B, A, 0); /* |a| < |b|, отрицательный результат */
  rc |= test_sub_alias(E, B, 0); /* заём через границу слова */

  /* sub, алиасинг res == val_2 */
  rc |= test_sub_alias(A, B, 1);
  rc |= test_sub_alias(B, A, 1);
  rc |= test_sub_alias(E, B, 1);

  /* sub, все три равны */
  rc |= test_sub_alias(A, A, 2);
  rc |= test_sub_alias(D, D, 2);

  if (rc == 0)
    fprintf(stderr, "ALL ALIAS TESTS PASSED\n");
  else
    fprintf(stderr, "ALIAS TESTS FAILED\n");

  return rc;
}

int run_bignum_test(const bignum_operation_t *ops, size_t count)
{
  printf("------------------RUNNING BIGNUM TEST------------------\n");
  for (size_t i = 0; i < count; i++)
  {
    const bignum_operation_t *op = &ops[i];

    bignum_t a, b, res, q, r, mod;
    bignum_init(&a);
    bignum_init(&b);
    bignum_init(&res);
    bignum_init(&q);
    bignum_init(&r);
    bignum_init(&mod);

    bignum_from_hex_string(&a, op->a, op->neg_a);
    bignum_from_hex_string(&b, op->b, op->neg_b);
    if (op->mod[0] != '\0')
    {
      bignum_from_hex_string(&mod, op->mod, false);
    }

    int err = 0;
    if (strcmp(op->op, "add") == 0)
    {
      err = bignum_add(&res, &a, &b);
      if (err == 0)
      {
        bignum_print_dec(&a);
        printf(" + ");
        bignum_print_dec(&b);
        printf(" = ");
        bignum_print_dec(&res);
        printf("\n");
      }
    }
    else if (strcmp(op->op, "sub") == 0)
    {
      err = bignum_sub(&res, &a, &b);
      if (err == 0)
      {
        bignum_print_dec(&a);
        printf(" - ");
        bignum_print_dec(&b);
        printf(" = ");
        bignum_print_dec(&res);
        printf("\n");
      }
    }
    else if (strcmp(op->op, "mul") == 0)
    {
      err = bignum_mul(&res, &a, &b);
      if (err == 0)
      {
        bignum_print_dec(&a);
        printf(" * ");
        bignum_print_dec(&b);
        printf(" = ");
        bignum_print_dec(&res);
        printf("\n");
      }
    }
    else if (strcmp(op->op, "divmod") == 0)
    {
      err = bignum_divmod(&q, &r, &a, &b);
      if (err == 0)
      {
        bignum_print_dec(&a);
        printf(" / ");
        bignum_print_dec(&b);
        printf(" = ");
        bignum_print_dec(&q);
        printf(", remainder ");
        bignum_print_dec(&r);
        printf("\n");
      }
    }
    else if (strcmp(op->op, "pow") == 0)
    {
      err = bignum_pow(&res, &a, &b);
      if (err == 0)
      {
        bignum_print_dec(&a);
        printf(" ^ ");
        bignum_print_dec(&b);
        printf(" = ");
        bignum_print_dec(&res);
        printf("\n");
      }
    }
    else if (strcmp(op->op, "pow_mod") == 0)
    {
      err = bignum_pow_mod(&res, &a, &b, &mod);
      if (err == 0)
      {
        bignum_print_dec(&a);
        printf(" ^ ");
        bignum_print_dec(&b);
        printf(" mod ");
        bignum_print_dec(&mod);
        printf(" = ");
        bignum_print_dec(&res);
        printf("\n");
      }
    }
    else if (strcmp(op->op, "gcd") == 0)
    {
      err = bignum_gcd(&res, &a, &b);
      if (err == 0)
      {
        printf("gcd(");
        bignum_print_dec(&a);
        printf(", ");
        bignum_print_dec(&b);
        printf(") = ");
        bignum_print_dec(&res);
        printf("\n");
      }
    }
    else if (strcmp(op->op, "lcm") == 0)
    {
      err = bignum_lcm(&res, &a, &b);
      if (err == 0)
      {
        printf("lcm(");
        bignum_print_dec(&a);
        printf(", ");
        bignum_print_dec(&b);
        printf(") = ");
        bignum_print_dec(&res);
        printf("\n");
      }
    }
    else if (strcmp(op->op, "cmp") == 0)
    {
      printf("cmp(");
      bignum_print_dec(&a);
      printf(", ");
      bignum_print_dec(&b);
      printf(") = %d\n", bignum_cmp(&a, &b));
    }
    else
    {
      printf("unknown op: %s\n", op->op);
    }

    if (err < 0)
      printf("  error: %d\n", err);

    bignum_delete(&a);
    bignum_delete(&b);
    bignum_delete(&res);
    bignum_delete(&q);
    bignum_delete(&r);
    bignum_delete(&mod);
  }
  return 0;
}
