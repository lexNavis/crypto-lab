#include "../primality/poly.h"
#include "../bignum/bignum.h"
#include <stdio.h>

/* Вспомогательная: проверить, что полином p имеет коэффициенты,
 * заданные в массиве expected (длина r). Возвращает 0 если совпало. */
static int check_poly_coeffs(const poly_t *p, const uint32_t *expected, const char *label)
{
  for (size_t i = 0; i < p->r; i++)
  {
    uint32_t got = 0;
    /* Коэффициенты маленькие (< 2^32) в тестах, берём младшее слово */
    bignum_to_ui32_array(&got, 1, &p->coeffs[i]);
    if (got != expected[i])
    {
      fprintf(stderr, "FAIL %s: coeff[%zu] = %u, expected %u\n",
              label, i, got, expected[i]);
      return -1;
    }
  }
  return 0;
}

/* Установить коэффициенты полинома из массива uint32_t */
static int set_coeffs(poly_t *p, const uint32_t *vals)
{
  bignum_t tmp;
  bignum_init(&tmp);
  for (size_t i = 0; i < p->r; i++)
  {
    int err = bignum_from_ui32_array(&tmp, &vals[i], 1, false);
    if (err < 0) { bignum_delete(&tmp); return err; }
    err = bignum_copy(&p->coeffs[i], &tmp);
    if (err < 0) { bignum_delete(&tmp); return err; }
  }
  bignum_delete(&tmp);
  return 0;
}

static int test_poly_add(void)
{
  /* r = 4, a = x + 1, b = x² + 1, a + b = x² + x + 2 */
  poly_t a, b, res;
  poly_init(&a, 4);
  poly_init(&b, 4);
  poly_init(&res, 4);

  const uint32_t av[] = {1, 1, 0, 0};
  const uint32_t bv[] = {1, 0, 1, 0};
  set_coeffs(&a, av);
  set_coeffs(&b, bv);

  bignum_t n;
  bignum_init(&n);
  bignum_from_ui32_array(&n, (uint32_t[]){100}, 1, false);  /* mod 100 */

  int rc = 0;
  int err = poly_add(&res, &a, &b, &n);
  if (err < 0) { fprintf(stderr, "FAIL poly_add: err=%d\n", err); rc = -1; }
  else {
    const uint32_t exp[] = {2, 1, 1, 0};
    rc = check_poly_coeffs(&res, exp, "poly_add");
    if (rc == 0) fprintf(stderr, "poly_add — OK\n");
  }

  bignum_delete(&n);
  poly_delete(&a);
  poly_delete(&b);
  poly_delete(&res);
  return rc;
}

static int test_poly_mul_no_reduction(void)
{
  /* r = 4, a = x + 1, b = x + 1, a * b = x² + 2x + 1 */
  poly_t a, b, res;
  poly_init(&a, 4);
  poly_init(&b, 4);
  poly_init(&res, 4);

  const uint32_t av[] = {1, 1, 0, 0};
  set_coeffs(&a, av);
  set_coeffs(&b, av);

  bignum_t n;
  bignum_init(&n);
  bignum_from_ui32_array(&n, (uint32_t[]){100}, 1, false);

  int rc = 0;
  int err = poly_mul(&res, &a, &b, &n);
  if (err < 0) { fprintf(stderr, "FAIL poly_mul: err=%d\n", err); rc = -1; }
  else {
    const uint32_t exp[] = {1, 2, 1, 0};
    rc = check_poly_coeffs(&res, exp, "poly_mul (no reduction)");
    if (rc == 0) fprintf(stderr, "poly_mul (no reduction) — OK\n");
  }

  bignum_delete(&n);
  poly_delete(&a);
  poly_delete(&b);
  poly_delete(&res);
  return rc;
}

static int test_poly_mul_with_reduction(void)
{
  /* r = 4, a = x³, b = x², a * b = x^5 ≡ x^1 (mod x^4 - 1) */
  poly_t a, b, res;
  poly_init(&a, 4);
  poly_init(&b, 4);
  poly_init(&res, 4);

  const uint32_t av[] = {0, 0, 0, 1};  /* x³ */
  const uint32_t bv[] = {0, 0, 1, 0};  /* x² */
  set_coeffs(&a, av);
  set_coeffs(&b, bv);

  bignum_t n;
  bignum_init(&n);
  bignum_from_ui32_array(&n, (uint32_t[]){100}, 1, false);

  int rc = 0;
  int err = poly_mul(&res, &a, &b, &n);
  if (err < 0) { fprintf(stderr, "FAIL poly_mul (reduction): err=%d\n", err); rc = -1; }
  else {
    const uint32_t exp[] = {0, 1, 0, 0};  /* x^1 */
    rc = check_poly_coeffs(&res, exp, "poly_mul (reduction)");
    if (rc == 0) fprintf(stderr, "poly_mul (reduction) — OK\n");
  }

  bignum_delete(&n);
  poly_delete(&a);
  poly_delete(&b);
  poly_delete(&res);
  return rc;
}

static int test_poly_pow(void)
{
  /* r = 4, a = x + 1, exp = 3, a³ = x³ + 3x² + 3x + 1 */
  poly_t a, res;
  poly_init(&a, 4);
  poly_init(&res, 4);

  const uint32_t av[] = {1, 1, 0, 0};
  set_coeffs(&a, av);

  bignum_t n, exp;
  bignum_init(&n);
  bignum_init(&exp);
  bignum_from_ui32_array(&n, (uint32_t[]){1000}, 1, false);
  bignum_from_ui32_array(&exp, (uint32_t[]){3}, 1, false);

  int rc = 0;
  int err = poly_pow(&res, &a, &exp, &n);
  if (err < 0) { fprintf(stderr, "FAIL poly_pow: err=%d\n", err); rc = -1; }
  else {
    const uint32_t expected[] = {1, 3, 3, 1};  /* x³ + 3x² + 3x + 1 */
    rc = check_poly_coeffs(&res, expected, "poly_pow");
    if (rc == 0) fprintf(stderr, "poly_pow — OK\n");
  }

  bignum_delete(&n);
  bignum_delete(&exp);
  poly_delete(&a);
  poly_delete(&res);
  return rc;
}

int run_poly_test(void)
{
  int rc = 0;
  rc |= test_poly_add();
  rc |= test_poly_mul_no_reduction();
  rc |= test_poly_mul_with_reduction();
  rc |= test_poly_pow();

  if (rc == 0) fprintf(stderr, "ALL POLY TESTS PASSED\n");
  else fprintf(stderr, "POLY TESTS FAILED\n");
  return rc;
}