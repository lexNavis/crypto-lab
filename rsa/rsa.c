#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include "prime.h"
#include "rsa.h"

typedef struct
{
  mpz_t val;
  unsigned int bits;
  int err;
} generate_prime_t;

static void *generate_prime_thread(void *arg) {
  generate_prime_t *data = (generate_prime_t *)arg;
  data->err = generate_prime(data->val, data->bits);
  return NULL;
}

static int path_to_prime(char *buf, size_t buf_size, unsigned int bits)
{
  int written = snprintf(buf, buf_size, "./rsa/prime_numbers/p_%u.txt", bits);
  if (written < 0)
  {
    /* Handle error */
    return -ERR_SNPRINTF_FAILED;
  }
  if ((size_t)written >= buf_size)
  {
    /* Handle error */
    return -ERR_BUFFER_TOO_SMALL;
  }
  return 0;
}


void rsa_key_init(rsa_key_t *key)
{
  mpz_init(key->d);
  mpz_init(key->e);
  mpz_init(key->n);
}

void rsa_key_clear(rsa_key_t *key)
{
  mpz_clear(key->d);
  mpz_clear(key->e);
  mpz_clear(key->n);
}

int rsa_keygen(rsa_key_t *key, unsigned int bits, bool from_file)
{
  int err = 0;
  if (bits < 16)
  {
    /* Handle error */
    err = -ERR_RSA_SIZE_TOO_SMALL;
    goto out;
  }
  generate_prime_t p, q;
  mpz_init(p.val);
  mpz_init(q.val);
  p.bits = bits/2;
  q.bits = bits - p.bits;
  p.err = q.err = 0;
  if (!from_file) {
    pthread_t t1, t2;
    int err_p = pthread_create(&t1, NULL, generate_prime_thread, &p);
    if (err_p != 0) {
      err = err_p;
      goto clear_pq;
    }
    int err_q = pthread_create(&t2, NULL, generate_prime_thread, &q);
    if (err_q != 0) {
      err = err_q;
      goto clear_pq;
    }
    err_p = pthread_join(t1, NULL);
    if (err_p != 0) {
      err = err_p;
      goto clear_pq;
    }
    else if (p.err != 0) {
      err = p.err;
      goto clear_pq;
    }
    gmp_printf("p = %Zd\n", p.val);
    err_q =pthread_join(t2, NULL);
    if (err_q != 0) {
      err = err_q;
      goto clear_pq;
    }
    else if (q.err != 0) {
      err = q.err;
      goto clear_pq;
    }
    gmp_printf("q = %Zd\n", q.val);
    /* Save this pair to a file */
    size_t buf_size = 256;
    char path_buf[buf_size];
    int path_err = path_to_prime(path_buf, buf_size, bits/2);
    if (path_err < 0) {
      /* Handle error */
      err = path_err;
      goto clear_pq;
    }
    FILE *f = fopen(path_buf, "w");
    if (!f) {
        err = -ERR_WRITE_TO_FILE_FAILED;
        goto clear_pq;
    }
    mpz_out_str(f, 10, p.val);
    fprintf(f, "\n");
    mpz_out_str(f, 10, q.val);
    fprintf(f, "\n");
    fclose(f);
  }
  else {
    size_t buf_size = 256;
    char path_buf[buf_size];
    /**
     * @bug Известно, что чтение из файла простых чисел для нечетного bits 
     * приведет к тому, что эти простые числа при перемножении дадут 
     * модуль размером bits+1. Да, знаем, может, исправим.
     */
    int path_err = path_to_prime(path_buf, buf_size, bits/2);
    if (path_err < 0) {
      /* Handle error */
      err = path_err;
      goto clear_pq;
    }
    FILE *f = fopen(path_buf, "r");
    if (!f) {
      err = -ERR_FILE_NOT_FOUND;
      goto clear_pq;
    }
    if (mpz_inp_str(p.val, f, 10) == 0) {
      fclose(f);
      err = -ERR_READ_FROM_FILE_FAILED;
      goto clear_pq;
    }
    if (mpz_inp_str(q.val, f, 10) == 0) {
      fclose(f);
      err = -ERR_READ_FROM_FILE_FAILED;
      goto clear_pq;
    }
    fclose(f);
  }
  /* Main algorithm */
  if (mpz_cmp(p.val, q.val) == 0) {
    err = -ERR_EQUAL_PRIMES;
    goto clear_pq;
  }
  rsa_key_init(key);
  /* Count n = p * q */
  mpz_mul(key->n, p.val, q.val);
  /* Count Euler function phi(n) = (p-1)(q-1) 
   * Save original primes p and q
   */
  mpz_t p_minus_1, q_minus_1;
  mpz_init(p_minus_1);
  mpz_init(q_minus_1);
  mpz_sub_ui(p_minus_1, p.val, 1);
  mpz_sub_ui(q_minus_1, q.val, 1);
  mpz_t phi;
  mpz_init(phi);
  mpz_mul(phi, p_minus_1, q_minus_1);
  /* Init e */
  mpz_set_ui(key->e, 65537);
  /* Count d */
  if (mpz_invert(key->d, key->e, phi) == 0) {
    /* Zero return means that d couldn't be counted. */
    err = -ERR_INVERT_FAILED;
    goto clear_all;
  }
  clear_all:
    mpz_clear(p_minus_1);
    mpz_clear(q_minus_1);
    mpz_clear(phi);
  clear_pq:
    mpz_clear(p.val);
    mpz_clear(q.val);
  out:
    return err;
}

int rsa_encrypt(const mpz_t val, mpz_t encrypted_val, const rsa_key_t *key) {
  if (mpz_cmp(val, key->n) >= 0) {
    return -ERR_MSG_TOO_BIG;
  }
  mpz_powm(encrypted_val, val, key->e, key->n);
  return 0;
}

int rsa_decrypt(const mpz_t val, mpz_t decrypted_val, const rsa_key_t *key) {
  mpz_powm(decrypted_val, val, key->d, key->n);
  return 0;
}
