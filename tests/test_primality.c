#include "../primality/primality.h"
#include "../bignum/bignum.h"
#include "../mersen/mersen.h"
#include <stdio.h>
#include <time.h>

static double elapsed(const struct timespec *t0, const struct timespec *t1)
{
  return (t1->tv_sec - t0->tv_sec) + (t1->tv_nsec - t0->tv_nsec) / 1e9;
}

/* 100 прогонов Миллера-Рабина на известном простом числе.
 * Собирает статистику: сколько раз "простое", сколько "составное",
 * среднее/мин/макс время. */
static int check_miller_rabin_stat(const bignum_t *N, const char *label, int rounds)
{
  int prime_count = 0, composite_count = 0, err_count = 0;
  double t_min = 1e9, t_max = 0.0, t_sum = 0.0;

  for (int i = 0; i < rounds; i++)
  {
    /* Случайная база в [2, N-2] */
    bignum_t nm3, base;
    bignum_init(&nm3);
    bignum_init(&base);
    int err = bignum_sub_digit(&nm3, N, 3);  // N-3
    if (err < 0) { err_count++; bignum_delete(&nm3); bignum_delete(&base); continue; }
    err = bignum_rand_range(&base, &nm3);    // [0, N-4]
    if (err < 0) { err_count++; bignum_delete(&nm3); bignum_delete(&base); continue; }
    err = bignum_add_digit(&base, &base, 2);  // [2, N-2]
    if (err < 0) { err_count++; bignum_delete(&nm3); bignum_delete(&base); continue; }
    bignum_delete(&nm3);

    bool is_prime = false;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    err = mersen_miller_rabin(&is_prime, N, &base);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    bignum_delete(&base);

    if (err < 0) { err_count++; continue; }
    double sec = elapsed(&t0, &t1);
    if (sec < t_min) t_min = sec;
    if (sec > t_max) t_max = sec;
    t_sum += sec;
    if (is_prime) prime_count++; else composite_count++;
  }

  double avg = (rounds - err_count > 0) ? t_sum / (rounds - err_count) : 0.0;
  fprintf(stderr, "%s: %d rounds — prime=%d, composite=%d, err=%d\n",
          label, rounds, prime_count, composite_count, err_count);
  fprintf(stderr, "  time: min=%.6f, avg=%.6f, max=%.6f s\n", t_min, avg, t_max);

  if (prime_count != rounds) {
    fprintf(stderr, "FAIL %s: expected all %d rounds to say prime\n", label, rounds);
    return -1;
  }
  return 0;
}

static int check_lucas_strong(void)
{
  /* Пары (exp, ожидание) для M_exp = 2^exp - 1 */
  const struct { uint32_t exp; bool expected; } cases[] = {
    { 2,  true  },  /* M_2 = 3 */
    { 3,  true  },  /* M_3 = 7 */
    { 5,  true  },  /* M_5 = 31 */
    { 7,  true  },  /* M_7 = 1279 */
    { 11, false },  /* M_11 = 2047 = 23 * 89 */
    { 13, true  },  /* M_13 = 8191 */
    { 17, true  },  /* M_17 = 131071 */
    { 19, true  },  /* M_19 = 524287 */
    { 23, false },  /* M_23 = 47 * 178481 */
    { 29, false },  /* M_29 = 233 * 1103 * 2089 */
    { 31, true  },  /* M_31 = 2147483647 */
  };

  int rc = 0;
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
  {
    bignum_t M;
    bignum_init(&M);
    int err = form_mersen_number(&M, cases[i].exp);
    if (err < 0) {
      fprintf(stderr, "FAIL form_mersen_number(exp=%u): err=%d\n", cases[i].exp, err);
      rc = -1;
      bignum_delete(&M);
      continue;
    }

    bool is_prime = false;
    err = primality_lucas_strong(&is_prime, &M);
    if (err < 0) {
      fprintf(stderr, "FAIL lucas_strong(exp=%u): err=%d\n", cases[i].exp, err);
      rc = -1;
    } else if (is_prime != cases[i].expected) {
      fprintf(stderr, "FAIL lucas_strong(exp=%u): got %s, expected %s\n",
              cases[i].exp, is_prime ? "prime" : "composite",
              cases[i].expected ? "prime" : "composite");
      rc = -1;
    } else {
      fprintf(stderr, "lucas_strong(exp=%u): %s — OK\n",
              cases[i].exp, is_prime ? "prime" : "composite");
    }
    bignum_delete(&M);
  }
  return rc;
}

static int check_bpsw_pseudoprimes(void)
{
  /* Все числа — составные, BPSW должен сказать "composite".
   * Группы:
   *   - Кармайкла (A002997)
   *   - сильные псевдопростые по базе 2 (A001262)
   *   - сильные псевдопростые Люка (A217255)
   *   - просто большие составные с маленькими делителями
   */
  const uint32_t composites[] = {
    /* Кармайкла */
    561, 1105, 1729, 2465, 2821, 6601, 8911, 10585,
    /* Сильные псевдопростые по базе 2 */
    2047, 3277, 4033, 4681, 8321, 15841, 29341,
    /* Сильные псевдопростые Люка */
    5459, 5777, 10877, 16109, 18971,
    /* Просто составные */
    9, 15, 21, 25, 27, 33, 35, 49, 77, 91, 121, 143,
    1001, 10001, 65535, 65537 * 3,
  };

  int rc = 0;
  for (size_t i = 0; i < sizeof(composites) / sizeof(composites[0]); i++)
  {
    bignum_t n;
    bignum_init(&n);
    int err = bignum_from_ui32_array(&n, (uint32_t[]){composites[i]}, 1, false);
    if (err < 0) {
      fprintf(stderr, "FAIL from_ui32_array(%u): err=%d\n", composites[i], err);
      rc = -1;
      bignum_delete(&n);
      continue;
    }

    bool is_prime = true;  /* намеренно true, чтобы поймать "простое" */
    err = primality_bpsw(&is_prime, &n);
    if (err < 0) {
      fprintf(stderr, "FAIL bpsw(%u): err=%d\n", composites[i], err);
      rc = -1;
    } else if (is_prime) {
      fprintf(stderr, "FAIL bpsw(%u): got prime, expected composite\n", composites[i]);
      rc = -1;
    } else {
      fprintf(stderr, "bpsw(%u): composite — OK\n", composites[i]);
    }
    bignum_delete(&n);
  }
  return rc;
}

/* Статистика для детерминированного теста: 100 прогонов. */
static int check_stat_100(const char *name,
                          int (*fn)(bool *, const bignum_t *),
                          const bignum_t *N, int rounds, bool expected)
{
  int prime_count = 0, composite_count = 0, err_count = 0;
  double t_min = 1e9, t_max = 0.0, t_sum = 0.0;

  for (int i = 0; i < rounds; i++)
  {
    bool is_prime = false;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    int err = fn(&is_prime, N);
    clock_gettime(CLOCK_MONOTONIC, &t1);

    if (err < 0) { err_count++; continue; }
    double sec = elapsed(&t0, &t1);
    if (sec < t_min) t_min = sec;
    if (sec > t_max) t_max = sec;
    t_sum += sec;
    if (is_prime) prime_count++; else composite_count++;
  }

  int ok = rounds - err_count;
  double avg = (ok > 0) ? t_sum / ok : 0.0;
  fprintf(stderr, "%s: %d rounds — prime=%d, composite=%d, err=%d\n",
          name, rounds, prime_count, composite_count, err_count);
  fprintf(stderr, "  time: min=%.6f, avg=%.6f, max=%.6f s\n", t_min, avg, t_max);

  if (err_count != 0) {
    fprintf(stderr, "FAIL %s: %d errors\n", name, err_count);
    return -1;
  }
  if (expected && prime_count != rounds) {
    fprintf(stderr, "FAIL %s: expected all prime, got %d\n", name, prime_count);
    return -1;
  }
  if (!expected && composite_count != rounds) {
    fprintf(stderr, "FAIL %s: expected all composite, got %d\n", name, composite_count);
    return -1;
  }
  return 0;
}

int run_primality_test(void)
{
  int rc = 0;

  /* Известное простое: 2^1279 - 1 (M_1279, простое Мерсенна) */
  bignum_t N;
  bignum_init(&N);
  int err = form_mersen_number(&N, 1279);
  if (err < 0) {
    fprintf(stderr, "FAIL form_mersen_number(1279): err=%d\n", err);
    bignum_delete(&N);
    return -1;
  }
  fprintf(stderr, "Testing known prime 2^1279 - 1 (length=%zu words)\n", bignum_length(&N));

  rc |= check_miller_rabin_stat(&N, "miller_rabin(2^1279-1)", 100);
  /* Пункт 2: Люк 100 раз на известном простом */
  rc |= check_stat_100("lucas_strong(2^1279-1)", primality_lucas_strong, &N, 100, true);

  /* Пункт 3: BPSW 100 раз на известном простом */
  rc |= check_stat_100("bpsw(2^1279-1)", primality_bpsw, &N, 100, true);

  /* Пункт 4: сравнение трёх тестов на одном числе */
  {
    bool mr = false, lucas = false, bpsw = false;
    struct timespec t0, t1;
    double t_mr, t_lucas, t_bpsw;

    bignum_t two_bn;
    bignum_init(&two_bn);
    bignum_from_ui32_array(&two_bn, (uint32_t[]){2}, 1, false);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    mersen_miller_rabin(&mr, &N, &two_bn);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    t_mr = elapsed(&t0, &t1);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    primality_lucas_strong(&lucas, &N);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    t_lucas = elapsed(&t0, &t1);

    clock_gettime(CLOCK_MONOTONIC, &t0);
    primality_bpsw(&bpsw, &N);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    t_bpsw = elapsed(&t0, &t1);

    bignum_delete(&two_bn);

    fprintf(stderr, "\n=== Comparison on 2^1279-1 ===\n");
    fprintf(stderr, "Miller-Rabin(2): %-10s %.6f s\n", mr ? "prime" : "composite", t_mr);
    fprintf(stderr, "Lucas strong:    %-10s %.6f s\n", lucas ? "prime" : "composite", t_lucas);
    fprintf(stderr, "BPSW:            %-10s %.6f s\n", bpsw ? "prime" : "composite", t_bpsw);

    if (!mr || !lucas || !bpsw) {
      fprintf(stderr, "FAIL comparison: not all tests say prime\n");
      rc = -1;
    }
  }

  bignum_delete(&N);

  if (rc == 0) fprintf(stderr, "ALL PRIMALITY TESTS PASSED\n");
  else fprintf(stderr, "PRIMALITY TESTS FAILED\n");
  return rc;
}