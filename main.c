#include <stdio.h>
#include <string.h>
#include "common/config.h"

int run_kuz_test(bool use_random_key, const char *custom_block);
int run_streebog_test(const char *custom_message);
int run_rsa_test(unsigned int bits, bool from_file, const char *message);
int run_bignum_test(void);

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s <rsa|kuz|streebog|bignum|all>\n", argv[0]);
    return 1;
  }
  config_t cfg;
  /* Получение параметров из конфиг файла */
  int err = load_config("config.json", &cfg);
  if (err < 0)
  {
    return err;
  }
  if (strcmp(argv[1], "rsa") == 0)
  {
    return run_rsa_test(cfg.rsa.bits, cfg.rsa.from_file, cfg.rsa.message);
  }
  if (strcmp(argv[1], "kuz") == 0)
  {
    return run_kuz_test(cfg.kuznyechik.use_random_key, cfg.kuznyechik.block);
  }
  if (strcmp(argv[1], "streebog") == 0)
  {
    return run_streebog_test(cfg.streebog.message);
  }
  if (strcmp(argv[1], "bignum") == 0)
  {
    return run_bignum_test();
  }
  if (strcmp(argv[1], "all") == 0)
  {
    int rc = 0;
    rc |= run_kuz_test(cfg.kuznyechik.use_random_key, cfg.kuznyechik.block);
    rc |= run_streebog_test(cfg.streebog.message);
    rc |= run_rsa_test(cfg.rsa.bits, cfg.rsa.from_file, cfg.rsa.message);
    rc |= run_bignum_test();
    return rc;
  }

  fprintf(stderr, "Unknown mode: %s\n", argv[1]);
  return 1;
}