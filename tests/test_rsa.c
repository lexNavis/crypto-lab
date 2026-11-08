#include "../rsa/rsa.h"
#include <gmp.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* GMP-версия: как было. */
static int run_rsa_test_gmp(unsigned int bits, bool from_file,
                            const char *message) {
  printf("------------------RUNNING RSA (GMP)------------------\n");
  rsa_key_t key;
  rsa_key_init(&key);

  int err = rsa_keygen(&key, bits, from_file);
  if (err != 0) {
    fprintf(stderr, "rsa_keygen failed: %d\n", err);
    rsa_key_clear(&key);
    return 1;
  }

  mpz_t val, encrypted_val, decrypted_val;
  mpz_init(val);
  mpz_init(encrypted_val);
  mpz_init(decrypted_val);
  err = mpz_set_str(val, message, 10);
  if (err != 0) {
    fprintf(stderr, "Invalid message: %s\n", message);
    goto cleanup;
  }

  err = rsa_encrypt(val, encrypted_val, &key);
  if (err != 0) {
    fprintf(stderr, "encrypt failed: %d\n", err);
    goto cleanup;
  }

  err = rsa_decrypt(encrypted_val, decrypted_val, &key);
  if (err != 0) {
    fprintf(stderr, "decrypt failed: %d\n", err);
    goto cleanup;
  }

  if (mpz_cmp(val, decrypted_val) == 0)
    printf("OK (GMP): decrypted == original\n");
  else
    printf("FAIL (GMP): decrypted != original\n");

cleanup:
  mpz_clear(val);
  mpz_clear(encrypted_val);
  mpz_clear(decrypted_val);
  rsa_key_clear(&key);
  return err;
}

/* Сравнение GMP и BigNum на одном ключе (p, q из файла).
 * Проверяем: n, e, d совпадают; c и m2 совпадают. */
static int run_rsa_test_compare(unsigned int bits, bool from_file, const char *message)
{
  printf("------------------RUNNING RSA (GMP vs BigNum)------------------\n");

  rsa_key_t gmp_key;
  rsa_key_init(&gmp_key);
  int err = rsa_keygen(&gmp_key, bits, from_file);
  if (err != 0) {
    fprintf(stderr, "rsa_keygen (GMP) failed: %d\n", err);
    rsa_key_clear(&gmp_key);
    return 1;
  }

  rsa_key_bignum_t bn_key;
  rsa_key_bignum_init(&bn_key);
  err = rsa_keygen_bignum(&bn_key, bits, from_file);
  if (err != 0) {
    fprintf(stderr, "rsa_keygen_bignum failed: %d\n", err);
    rsa_key_clear(&gmp_key);
    rsa_key_bignum_clear(&bn_key);
    return 1;
  }

  int rc = 0;
  mpz_t tmp;
  mpz_init(tmp);

  /* n, e, d */
  bignum_to_gmp(tmp, &bn_key.n);
  if (mpz_cmp(tmp, gmp_key.n) != 0) { fprintf(stderr, "FAIL: n mismatch\n"); rc = -1; }
  bignum_to_gmp(tmp, &bn_key.e);
  if (mpz_cmp(tmp, gmp_key.e) != 0) { fprintf(stderr, "FAIL: e mismatch\n"); rc = -1; }
  bignum_to_gmp(tmp, &bn_key.d);
  if (mpz_cmp(tmp, gmp_key.d) != 0) { fprintf(stderr, "FAIL: d mismatch\n"); rc = -1; }
  mpz_clear(tmp);

  if (rc == 0) printf("OK: n, e, d match between GMP and BigNum\n");

  /* m */
  mpz_t m_gmp, c_gmp, m2_gmp;
  mpz_init(m_gmp); mpz_init(c_gmp); mpz_init(m2_gmp);
  if (mpz_set_str(m_gmp, message, 10) != 0) {
    fprintf(stderr, "Invalid message: %s\n", message);
    mpz_clear(m_gmp); mpz_clear(c_gmp); mpz_clear(m2_gmp);
    rsa_key_clear(&gmp_key);
    rsa_key_bignum_clear(&bn_key);
    return -1;
  }

  /* GMP encrypt/decrypt */
  err = rsa_encrypt(m_gmp, c_gmp, &gmp_key);
  if (err != 0) { fprintf(stderr, "GMP encrypt failed: %d\n", err); rc = -1; goto cleanup; }
  err = rsa_decrypt(c_gmp, m2_gmp, &gmp_key);
  if (err != 0) { fprintf(stderr, "GMP decrypt failed: %d\n", err); rc = -1; goto cleanup; }

  /* BigNum encrypt/decrypt */
  bignum_t m_bn, c_bn, m2_bn;
  bignum_init(&m_bn);
  bignum_init(&c_bn);
  bignum_init(&m2_bn);
  err = bignum_from_gmp(&m_bn, m_gmp);
  if (err < 0) { fprintf(stderr, "bignum_from_gmp(m) failed: %d\n", err); rc = -1; goto cleanup_bn; }
  err = rsa_encrypt_bignum(&m_bn, &c_bn, &bn_key);
  if (err < 0) { fprintf(stderr, "BigNum encrypt failed: %d\n", err); rc = -1; goto cleanup_bn; }
  err = rsa_decrypt_bignum(&c_bn, &m2_bn, &bn_key);
  if (err < 0) { fprintf(stderr, "BigNum decrypt failed: %d\n", err); rc = -1; goto cleanup_bn; }

  /* Сверка c и m2 через to_gmp */
  mpz_t c_bn_mpz, m2_bn_mpz;
  mpz_init(c_bn_mpz);
  mpz_init(m2_bn_mpz);
  bignum_to_gmp(c_bn_mpz, &c_bn);
  bignum_to_gmp(m2_bn_mpz, &m2_bn);

  if (mpz_cmp(c_gmp, c_bn_mpz) != 0) { fprintf(stderr, "FAIL: encrypted mismatch\n"); rc = -1; }
  else printf("OK: encrypted match\n");
  if (mpz_cmp(m2_gmp, m2_bn_mpz) != 0) { fprintf(stderr, "FAIL: decrypted mismatch\n"); rc = -1; }
  else printf("OK: decrypted match\n");
  if (mpz_cmp(m_gmp, m2_bn_mpz) == 0) printf("OK: BigNum decrypted == original\n");
  else { printf("FAIL: BigNum decrypted != original\n"); rc = -1; }

  /* ===== Печать: m (dec), e (dec), d (фрагменты hex), c (фрагменты hex), m2 (dec) ===== */

  printf("m (dec)    = "); mpz_out_str(stdout, 10, m_gmp); printf("\n");
  printf("e (dec)    = "); mpz_out_str(stdout, 10, gmp_key.e); printf("\n");

  /* d — фрагменты hex */
  char *d_hex = mpz_get_str(NULL, 16, gmp_key.d);
  size_t d_len = strlen(d_hex);
  printf("d (hex)    = ");
  if (d_len <= 32) printf("%s", d_hex);
  else printf("%.16s...%s (len=%zu)", d_hex, d_hex + d_len - 16, d_len);
  printf("\n");
  free(d_hex);

  /* c — фрагменты hex (GMP и BigNum) */
  char *c_gmp_hex = mpz_get_str(NULL, 16, c_gmp);
  char *c_bn_hex  = mpz_get_str(NULL, 16, c_bn_mpz);
  size_t c_gmp_len = strlen(c_gmp_hex);
  size_t c_bn_len  = strlen(c_bn_hex);
  printf("c (GMP)    = ");
  if (c_gmp_len <= 32) printf("%s", c_gmp_hex);
  else printf("%.16s...%s (len=%zu)", c_gmp_hex, c_gmp_hex + c_gmp_len - 16, c_gmp_len);
  printf("\n");
  printf("c (BigNum) = ");
  if (c_bn_len <= 32) printf("%s", c_bn_hex);
  else printf("%.16s...%s (len=%zu)", c_bn_hex, c_bn_hex + c_bn_len - 16, c_bn_len);
  printf("\n");
  free(c_gmp_hex);
  free(c_bn_hex);

  /* m2 — десятичное (BigNum) */
  printf("m2 (dec)   = "); bignum_print_dec(&m2_bn); printf("\n");

  mpz_clear(c_bn_mpz);
  mpz_clear(m2_bn_mpz);

cleanup_bn:
  bignum_delete(&m_bn);
  bignum_delete(&c_bn);
  bignum_delete(&m2_bn);

cleanup:
  mpz_clear(m_gmp);
  mpz_clear(c_gmp);
  mpz_clear(m2_gmp);
  rsa_key_clear(&gmp_key);
  rsa_key_bignum_clear(&bn_key);
  return rc;
}

int run_rsa_test(unsigned int bits, bool from_file, const char *message) {
  if (bits <= 32768 && from_file) {
    printf("Using BigNum + GMP comparison (bits=%u )\n", bits);
    return run_rsa_test_compare(bits, from_file, message);
  }
  printf("Using GMP only (bits=%u)\n", bits);
  return run_rsa_test_gmp(bits, from_file, message);
}