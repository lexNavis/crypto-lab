#include "prime.h"
#include "entropy.h"

int generate_prime(mpz_t val, unsigned int bits)
{
  int err;
  /* Both 0 and 1 are not prime */
  if (bits < 2)
  {
    return -ERR_INVALID_BITS;
  }
  if (bits > 100000) {
    return -ERR_BITS_TOO_BIG;
  }
  gmp_randstate_t state;
  gmp_randinit_default(state);
  err = seed_gmp_randstate(state);
  if (err < 0) {
    gmp_randclear(state);
    return err;
  }
  do
  {
    mpz_urandomb(val, state, bits);
    /* Set highest bit to 1 - guarantee, that val is correct size */
    mpz_setbit(val, bits - 1);
    /* Set lowest bit to 1 - guarantee, that val is not even */
    mpz_setbit(val, 0);
    /* Continue until we get prime number */
  } while (mpz_probab_prime_p(val, PRIME_TEST_CYCLES) == 0);

  gmp_randclear(state);
  return 0;
}