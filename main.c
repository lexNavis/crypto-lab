#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "common/config.h"

int run_kuz_test(bool use_random_key, const char *custom_block);
int run_streebog_test(const char *custom_message);
int run_rsa_test(unsigned int bits, bool from_file, const char *message);
int run_bignum_test(const bignum_operation_t *ops, size_t count);
int run_mersen_test(uint32_t exp);
int run_primality_test(void);
int run_poly_test(void);

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s <rsa|kuz|streebog|bignum|all>\n", argv[0]);
    return 1;
  }
  config_t cfg;
  int err = load_config("config.json", &cfg);
  if (err < 0)
    return 1;

  int rc = 0;
  if (strcmp(argv[1], "rsa") == 0)
  {
    rc = run_rsa_test(cfg.rsa.bits, cfg.rsa.from_file, cfg.rsa.message);
  }
  else if (strcmp(argv[1], "kuz") == 0)
  {
    rc = run_kuz_test(cfg.kuznyechik.use_random_key, cfg.kuznyechik.block);
  }
  else if (strcmp(argv[1], "streebog") == 0)
  {
    rc = run_streebog_test(cfg.streebog.message);
  }
  else if (strcmp(argv[1], "bignum") == 0)
  {
    rc = run_bignum_test(cfg.bignum.operations, cfg.bignum.count);
  }
  else if (strcmp(argv[1], "mersen") == 0)
  {
    rc = run_mersen_test(136279841);
  }
  else if (strcmp(argv[1], "primality") == 0)
  {
    rc = run_primality_test();
  }
  else if (strcmp(argv[1], "poly") == 0)
  {
    rc = run_poly_test();
  }
  else if (strcmp(argv[1], "all") == 0)
  {
    rc |= run_kuz_test(cfg.kuznyechik.use_random_key, cfg.kuznyechik.block);
    rc |= run_streebog_test(cfg.streebog.message);
    rc |= run_rsa_test(cfg.rsa.bits, cfg.rsa.from_file, cfg.rsa.message);
    rc |= run_bignum_test(cfg.bignum.operations, cfg.bignum.count);
    rc |= run_mersen_test(136279841);
    rc |= run_primality_test();
  }
  else
  {
    fprintf(stderr, "Unknown mode: %s\n", argv[1]);
    rc = 1;
  }

  config_free(&cfg);
  return rc;
}