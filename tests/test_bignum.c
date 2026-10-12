#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../bignum/bignum.h"
#include "../common/config.h"
#include "args.h"

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
