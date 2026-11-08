#include "../primality/primality.h"
#include "../primality/poly.h"
#include "../mersen/mersen.h"
#include "../bignum/bignum.h"
#include <stdio.h>
#include <time.h>

/* Эталонные значения для AKS */
static int check_aks(void)
{
  const struct {
    uint32_t exp;
    bool expected;
    const char *note;
  } cases[] = {
    /* < 2^32 — быстро */
    { 2,  true,  "M_2 = 3" },
    { 3,  true,  "M_3 = 7" },
    { 5,  true,  "M_5 = 31" },
    { 7,  true,  "M_7 = 127" },
    { 11, false, "M_11 = 2047 = 23*89" },
    { 13, true,  "M_13 = 8191" },
    { 17, true,  "M_17 = 131071" },
    { 19, true,  "M_19 = 524287" },
    { 23, false, "M_23 = 47*178481" },
    { 29, false, "M_29 = 233*1103*2089" },
    { 31, true,  "M_31 = 2147483647 (граница < 2^32)" },
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
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    err = primality_aks(&is_prime, &M);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double sec = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;

    if (err < 0) {
      fprintf(stderr, "FAIL aks(exp=%u, %s): err=%d\n", cases[i].exp, cases[i].note, err);
      rc = -1;
    } else if (is_prime != cases[i].expected) {
      fprintf(stderr, "FAIL aks(exp=%u, %s): got %s, expected %s (%.6f s)\n",
              cases[i].exp, cases[i].note,
              is_prime ? "prime" : "composite",
              cases[i].expected ? "prime" : "composite", sec);
      rc = -1;
    } else {
      fprintf(stderr, "aks(exp=%u, %s): %s — OK (%.6f s)\n",
              cases[i].exp, cases[i].note,
              is_prime ? "prime" : "composite", sec);
    }
    bignum_delete(&M);
  }
  return rc;
}

/* AKS для n > 2^64: берём маленькое (по битам) составное, чтобы
 * AKS быстро отсёк его на ранних шагах (полная степень или gcd). */
static int check_aks_large_composite(void)
{
  /* 2^65 + 1 = 36893488147419103233 — составное (делится на 3? проверим:
   * 2^65 mod 3 = 2, +1 = 0 mod 3, да, делится на 3) */
  bignum_t n;
  bignum_init(&n);

  /* n = 2^65 + 1 */
  int err = bignum_from_ui32_array(&n, (uint32_t[]){1}, 1, false);
  if (err < 0) { bignum_delete(&n); return -1; }
  err = bignum_shift_left_bits(&n, 65);
  if (err < 0) { bignum_delete(&n); return -1; }
  err = bignum_add_digit(&n, &n, 1);
  if (err < 0) { bignum_delete(&n); return -1; }

  bool is_prime = true;
  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  err = primality_aks(&is_prime, &n);
  clock_gettime(CLOCK_MONOTONIC, &t1);
  double sec = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;

  int rc = 0;
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
  return rc;
}

int run_aks_test(void)
{
  int rc = 0;

  /* AKS */
  rc |= check_aks();
  rc |= check_aks_large_composite();

  if (rc == 0) fprintf(stderr, "ALL POLY/AKS TESTS PASSED\n");
  else fprintf(stderr, "POLY/AKS TESTS FAILED\n");
  return rc;
}
