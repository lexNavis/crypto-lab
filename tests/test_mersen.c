#include "../mersen/mersen.h"
#include "../bignum/bignum.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <gmp.h>

/* ===== Замер времени ===== */

typedef int (*check_fn_2)(const bignum_t *, bool *);
typedef int (*check_fn_ll)(const bignum_t *, uint32_t, bool *);

static double elapsed(const struct timespec *t0, const struct timespec *t1)
{
  return (t1->tv_sec - t0->tv_sec) + (t1->tv_nsec - t0->tv_nsec) / 1e9;
}

static int time_check_2(const char *name, check_fn_2 fn,
                        const bignum_t *M, uint32_t exp, bool expected)
{
  bool is_prime = false;
  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  int err = fn(M, &is_prime);
  clock_gettime(CLOCK_MONOTONIC, &t1);
  double sec = elapsed(&t0, &t1);

  if (err < 0) {
    fprintf(stderr, "FAIL %s(exp=%u): err=%d\n", name, exp, err);
    return -1;
  }
  if (err > 0) {
    fprintf(stderr, "%s(exp=%u): %.6f s — not applicable\n", name, exp, sec);
    return 0;
  }
  if (is_prime != expected) {
    fprintf(stderr, "FAIL %s(exp=%u): got %s, expected %s (%.6f s)\n",
            name, exp, is_prime ? "prime" : "composite",
            expected ? "prime" : "composite", sec);
    return -1;
  }
  fprintf(stderr, "%s(exp=%u): %.6f s — %s OK\n",
          name, exp, sec, is_prime ? "prime" : "composite");
  return 0;
}

static int time_check_ll(const char *name, check_fn_ll fn,
                         const bignum_t *M, uint32_t exp, bool expected)
{
  bool is_prime = false;
  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  int err = fn(M, exp, &is_prime);
  clock_gettime(CLOCK_MONOTONIC, &t1);
  double sec = elapsed(&t0, &t1);

  if (err < 0) {
    fprintf(stderr, "FAIL %s(exp=%u): err=%d\n", name, exp, err);
    return -1;
  }
  if (err > 0) {
    fprintf(stderr, "%s(exp=%u): %.6f s — not applicable\n", name, exp, sec);
    return 0;
  }
  if (is_prime != expected) {
    fprintf(stderr, "FAIL %s(exp=%u): got %s, expected %s (%.6f s)\n",
            name, exp, is_prime ? "prime" : "composite",
            expected ? "prime" : "composite", sec);
    return -1;
  }
  fprintf(stderr, "%s(exp=%u): %.6f s — %s OK\n",
          name, exp, sec, is_prime ? "prime" : "composite");
  return 0;
}

/* ===== form_mersen_number с замером ===== */

static int time_form_mersen(bignum_t *M, uint32_t exp, double *out_sec)
{
  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  int err = form_mersen_number(M, exp);
  clock_gettime(CLOCK_MONOTONIC, &t1);
  *out_sec = elapsed(&t0, &t1);
  return err;
}

static int check_mersen_against_gmp(uint32_t exp)
{
  bignum_t M;
  bignum_init(&M);

  double sec = 0.0;
  int err = time_form_mersen(&M, exp, &sec);
  if (err < 0) {
    fprintf(stderr, "FAIL form_mersen_number(exp=%u): err=%d\n", exp, err);
    bignum_delete(&M);
    return -1;
  }
  fprintf(stderr, "form_mersen_number(exp=%u): %.6f s\n", exp, sec);

  mpz_t g, b;
  mpz_init(g);
  mpz_init(b);
  mpz_ui_pow_ui(g, 2, exp);
  mpz_sub_ui(g, g, 1);
  bignum_to_gmp(b, &M);

  int rc = 0;
  if (mpz_cmp(b, g) != 0) {
    fprintf(stderr, "FAIL form_mersen_number(exp=%u): mismatch\n", exp);
    rc = -1;
  }
  mpz_clear(b);
  mpz_clear(g);
  bignum_delete(&M);
  return rc;
}

/* ===== Прогоны методов на известных M_exp ===== */

typedef struct { uint32_t exp; bool expected; } mersen_case_t;

static const mersen_case_t cases[] = {
  { 2,  true  },  /* M_2 = 3 */
  { 3,  true  },  /* M_3 = 7 */
  { 5,  true  },  /* M_5 = 31 */
  { 7,  true  },  /* M_7 = 127 */
  { 11, false },  /* M_11 = 2047 = 23 * 89 */
  { 13, true  },  /* M_13 = 8191 */
  { 17, true  },  /* M_17 = 131071 */
  { 19, true  },  /* M_19 = 524287 */
  { 23, false },  /* M_23 = 47 * 178481 */
  { 29, false },  /* M_29 = 233 * 1103 * 2089 */
  { 31, true  },  /* M_31 = 2147483647 */
};

static int run_check_2(const char *name, check_fn_2 fn)
{
  int rc = 0;
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    bignum_t M;
    bignum_init(&M);
    int err = form_mersen_number(&M, cases[i].exp);
    if (err < 0) {
      fprintf(stderr, "FAIL form_mersen_number(exp=%u): err=%d\n", cases[i].exp, err);
      rc = -1;
    } else {
      rc |= time_check_2(name, fn, &M, cases[i].exp, cases[i].expected);
    }
    bignum_delete(&M);
  }
  return rc;
}

static int run_check_ll(void)
{
  int rc = 0;
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    bignum_t M;
    bignum_init(&M);
    int err = form_mersen_number(&M, cases[i].exp);
    if (err < 0) {
      fprintf(stderr, "FAIL form_mersen_number(exp=%u): err=%d\n", cases[i].exp, err);
      rc = -1;
    } else {
      rc |= time_check_ll("lucas_lehmer", mersen_check_lucas_lehmer,
                          &M, cases[i].exp, cases[i].expected);
    }
    bignum_delete(&M);
  }
  return rc;
}

/* ===== Основной раннер ===== */

int run_mersen_test(uint32_t exp)
{
  int rc = 0;

  /* Маленькие exp — полная сверка с GMP */
  const uint32_t small[] = { 1, 2, 3, 4, 7, 8, 13, 16, 31, 32, 33, 64, 127, 128, 129 };
  for (size_t i = 0; i < sizeof(small) / sizeof(small[0]); i++)
    rc |= check_mersen_against_gmp(small[i]);

  /* Большой exp — замер и проверка длины/старшего бита */
  {
    bignum_t M;
    bignum_init(&M);
    double sec = 0.0;
    int err = time_form_mersen(&M, exp, &sec);
    if (err < 0) {
      fprintf(stderr, "FAIL form_mersen_number(exp=%u): err=%d\n", exp, err);
      rc = -1;
    } else {
      fprintf(stderr, "form_mersen_number(exp=%u): %.6f s, length=%zu words\n",
              exp, sec, bignum_length(&M));
      size_t expected_words = (exp + 31) / 32;
      if (bignum_length(&M) != expected_words) {
        fprintf(stderr, "FAIL mersen(exp=%u): length=%zu, expected=%zu\n",
                exp, bignum_length(&M), expected_words);
        rc = -1;
      } else {
        bignum_t tmp;
        bignum_init(&tmp);
        bignum_copy(&tmp, &M);
        bignum_shift_right_bits(&tmp, exp - 1);
        if (!bignum_is_zero(&tmp) && tmp.val[0] == 1 && tmp.length == 1) {
          /* ок */
        } else {
          fprintf(stderr, "FAIL mersen(exp=%u): top bit not set\n", exp);
          rc = -1;
        }
        bignum_delete(&tmp);
      }
    }
    bignum_delete(&M);
  }

  /* Методы на известных M_exp */
  rc |= run_check_2("standard", mersen_check_standard);
  rc |= run_check_2("eratosthenes", mersen_check_eratosthenes);
  rc |= run_check_2("atkin", mersen_check_atkin);
  rc |= run_check_ll();

  if (rc == 0)
    fprintf(stderr, "ALL MERSEN TESTS PASSED\n");
  else
    fprintf(stderr, "MERSEN TESTS FAILED\n");

  return rc;
}