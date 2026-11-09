#include "../bignum/bignum.h"
#include "../mersen/mersen.h"
#include "../primality/primality.h"
#include "../deterministic/poly.h"
#include "../deterministic/deterministic.h"
#include <stdio.h>
#include <time.h>

static double elapsed(const struct timespec *t0, const struct timespec *t1) {
  return (t1->tv_sec - t0->tv_sec) + (t1->tv_nsec - t0->tv_nsec) / 1e9;
}

/* ===== Детерминированный Миллер ===== */

static int check_miller_det(void) {
  const struct { uint32_t exp; bool expected; } cases[] = {
      {2, true}, {3, true}, {5, true}, {7, true}, {11, false}, {13, true},
      {17, true}, {19, true}, {23, false}, {29, false}, {31, true},
      {32, false},  /* M_32 = 2^32 - 1 */
  };

  int rc = 0;
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
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
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    err = primality_miller_deterministic(&is_prime, &M);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double sec = elapsed(&t0, &t1);

    if (err < 0) {
      fprintf(stderr, "FAIL miller_det(exp=%u): err=%d\n", cases[i].exp, err);
      rc = -1;
    } else if (err > 0) {
      fprintf(stderr, "FAIL miller_det(exp=%u): not applicable\n", cases[i].exp);
      rc = -1;
    } else if (is_prime != cases[i].expected) {
      fprintf(stderr, "FAIL miller_det(exp=%u): got %s, expected %s (%.6f s)\n",
              cases[i].exp, is_prime ? "prime" : "composite",
              cases[i].expected ? "prime" : "composite", sec);
      rc = -1;
    } else {
      fprintf(stderr, "miller_det(exp=%u): %s — OK (%.6f s)\n", cases[i].exp,
              is_prime ? "prime" : "composite", sec);
    }
    bignum_delete(&M);
  }

  /* n > 2^64 → должно быть "не применимо" (err = 1) */
  {
    bignum_t M;
    bignum_init(&M);
    form_mersen_number(&M, 89);
    bool is_prime = false;
    int err = primality_miller_deterministic(&is_prime, &M);
    if (err != 1) {
      fprintf(stderr, "FAIL miller_det(exp=89): expected not applicable (err=1), got err=%d\n", err);
      rc = -1;
    } else {
      fprintf(stderr, "miller_det(exp=89): not applicable — OK (n > 2^64)\n");
    }
    bignum_delete(&M);
  }

  return rc;
}

/* ===== AKS ===== */

static int check_aks(void) {
  /* Для AKS берём только маленькие n — большие считаются часы/дни.
   * M_11, M_23, M_29 — составные (отсекаются на шаге gcd быстро).
   * M_3, M_5, M_7, M_13 — простые (AKS идёт до шага 5, M_13 ~ 18 с).
   * M_17, M_19, M_31 — не включаем (минуты/часы). */
  const struct { uint32_t exp; bool expected; const char *note; } cases[] = {
      {3,  true,  "M_3 = 7"},
      {5,  true,  "M_5 = 31"},
      {7,  true,  "M_7 = 127"},
      {11, false, "M_11 = 2047 = 23*89"},
      {13, true,  "M_13 = 8191 (~18 s)"},
      {23, false, "M_23 = 47*178481"},
      {29, false, "M_29 = 233*1103*2089"},
  };

  int rc = 0;
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
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
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    err = primality_aks(&is_prime, &M);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double sec = elapsed(&t0, &t1);

    if (err < 0) {
      fprintf(stderr, "FAIL aks(exp=%u, %s): err=%d\n", cases[i].exp, cases[i].note, err);
      rc = -1;
    } else if (is_prime != cases[i].expected) {
      fprintf(stderr, "FAIL aks(exp=%u, %s): got %s, expected %s (%.6f s)\n",
              cases[i].exp, cases[i].note, is_prime ? "prime" : "composite",
              cases[i].expected ? "prime" : "composite", sec);
      rc = -1;
    } else {
      fprintf(stderr, "aks(exp=%u, %s): %s — OK (%.6f s)\n",
              cases[i].exp, cases[i].note, is_prime ? "prime" : "composite", sec);
    }
    bignum_delete(&M);
  }

  /* AKS для n > 2^64: маленькое составное 2^65 + 1 (делится на 3),
   * отсекается на шаге gcd быстро. */
  {
    bignum_t n;
    bignum_init(&n);
    bignum_from_ui32_array(&n, (uint32_t[]){1}, 1, false);
    bignum_shift_left_bits(&n, 65);
    bignum_add_digit(&n, &n, 1);

    bool is_prime = true;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    int err = primality_aks(&is_prime, &n);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double sec = elapsed(&t0, &t1);

    if (err < 0) {
      fprintf(stderr, "FAIL aks(2^65+1): err=%d\n", err);
      rc = -1;
    } else if (is_prime) {
      fprintf(stderr, "FAIL aks(2^65+1): got prime, expected composite (%.6f s)\n", sec);
      rc = -1;
    } else {
      fprintf(stderr, "aks(2^65+1): composite — OK (%.6f s)\n", sec);
    }
    bignum_delete(&n);
  }

  return rc;
}

int run_deterministic_test(void) {
  int rc = 0;
  rc |= check_miller_det();
  rc |= check_aks();

  if (rc == 0)
    fprintf(stderr, "ALL DETERMINISTIC TESTS PASSED\n");
  else
    fprintf(stderr, "DETERMINISTIC TESTS FAILED\n");
  return rc;
}