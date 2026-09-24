#include "../common/entropy.h"
#include <sys/random.h>
int read_random_bytes(unsigned char *buf, size_t buf_size)
{
  ssize_t bytes_read = getrandom(buf, buf_size, 0);
  if (bytes_read < 0)
  {
    /* Handle error */
    return -ERR_GET_RANDOM_FAIL_TOTAL;
  }
  else if (bytes_read < buf_size)
  {
    /* Handle error */
    return -ERR_GET_RANDOM_FAIL_PARTIAL;
  }
  return 0;
}

int seed_gmp_randstate(gmp_randstate_t state)
{
  int err = 0;
  size_t buf_size = 32;
  unsigned char buf[buf_size];
  err = read_random_bytes(buf, buf_size);
  if (err < 0)
  {
    /* Handle error*/
    return err;
  }

  mpz_t seed;
  mpz_init(seed);
  /* buf -> seed: 32 bytes, big-endian (order=1), 1 byte per element. */
  mpz_import(seed, buf_size, 1, 1, 0, 0, buf);
  gmp_randseed(state, seed);
  mpz_clear(seed);
  return 0;
}