#include <stdio.h>
#include <stdbool.h>
#include <gmp.h>
#include "../rsa/rsa.h"

int run_rsa_test(unsigned int bits, bool from_file, const char *message)
{
  printf("------------------RUNNING RSA ALGOROTHM------------------\n");
  rsa_key_t key;
  rsa_key_init(&key);

  int err = rsa_keygen(&key, bits, from_file);
  if (err != 0)
  {
    fprintf(stderr, "rsa_keygen failed: %d\n", err);
    rsa_key_clear(&key);
    return 1;
  }

  gmp_printf("n = %Zd\n", key.n);
  gmp_printf("e = %Zd\n", key.e);
  gmp_printf("d = %Zd\n", key.d);

  mpz_t val, encrypted_val, decrypted_val;
  mpz_init(val);
  mpz_init(encrypted_val);
  mpz_init(decrypted_val);
  err = mpz_set_str(val, message, 10);
  if (err != 0)
  {
    fprintf(stderr, "Invalid message: %s\n", message);
    goto cleanup;
  }

  err = rsa_encrypt(val, encrypted_val, &key);
  if (err != 0)
  {
    fprintf(stderr, "encrypt failed: %d\n", err);
    goto cleanup;
  }

  err = rsa_decrypt(encrypted_val, decrypted_val, &key);
  if (err != 0)
  {
    fprintf(stderr, "decrypt failed: %d\n", err);
    goto cleanup;
  }

  gmp_printf("val  = %Zd\n", val);
  gmp_printf("encrypted_val  = %Zd\n", encrypted_val);
  gmp_printf("decrypted_val = %Zd\n", decrypted_val);

  if (mpz_cmp(val, decrypted_val) == 0)
    printf("OK: decrypted == original\n");
  else
    printf("FAIL: decrypted != original\n");

cleanup:
  mpz_clear(val);
  mpz_clear(encrypted_val);
  mpz_clear(decrypted_val);
  rsa_key_clear(&key);
  return err;
}