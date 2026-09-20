#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <gmp.h>
#include "rsa/rsa.h"

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <generate|load> <RSA key size>\n", argv[0]);
        return 1;
    }

    bool from_file;
    if (strcmp(argv[1], "load") == 0) {
        from_file = true;
    } else if (strcmp(argv[1], "generate") == 0) {
        from_file = false;
    } else {
        fprintf(stderr, "Unknown mode: %s\n", argv[1]);
        return 1;
    }

    unsigned int bits = (unsigned int)atoi(argv[2]);
    if (bits < 16) {
        fprintf(stderr, "bits must be >= 16\n");
        return 1;
    }
    rsa_key_t key;
    rsa_key_init(&key);

    int err = rsa_keygen(&key, bits, from_file);
    if (err != 0) {
        fprintf(stderr, "rsa_keygen failed: %d\n", err);
        rsa_key_clear(&key);
        return 1;
    }

    gmp_printf("n = %Zd\n", key.n);
    gmp_printf("e = %Zd\n", key.e);
    gmp_printf("d = %Zd\n", key.d);

    /* Тест: шифрование/дешифрование */
    mpz_t m, c, m2;
    mpz_init_set_ui(m, 12345);   /* сообщение */
    mpz_init(c);
    mpz_init(m2);

    err = rsa_encrypt(m, c, &key);
    if (err != 0) {
        fprintf(stderr, "rsa_encrypt failed: %d\n", err);
        goto cleanup;
    }

    err = rsa_decrypt(c, m2, &key);
    if (err != 0) {
        fprintf(stderr, "rsa_decrypt failed: %d\n", err);
        goto cleanup;
    }

    gmp_printf("m  = %Zd\n", m);
    gmp_printf("c  = %Zd\n", c);
    gmp_printf("m2 = %Zd\n", m2);

    if (mpz_cmp(m, m2) == 0) {
        printf("OK: decrypted == original\n");
    } else {
        printf("FAIL: decrypted != original\n");
    }

cleanup:
    mpz_clear(m);
    mpz_clear(c);
    mpz_clear(m2);
    rsa_key_clear(&key);
    return err != 0 ? 1 : 0;
}