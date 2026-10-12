#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../streebog/streebog.h"
#include "../common/hex.h"
#include "../bignum/bignum.h"
#include "args.h"

int run_streebog_test(const char *custom_message)
{
    printf("------------------RUNNING STREEBOG ALGORITHM------------------\n");
    const char *M_chars[] = { streebog_M1_char, streebog_M2_char, custom_message };
    size_t n = sizeof(M_chars) / sizeof(M_chars[0]);

    uint8_t hash[64];

    /* === Прямой вызов (байты) === */
    printf("\n=== Direct call (bytes) ===\n");
    for (size_t i = 0; i < n; i++) {
        size_t M_len;
        uint8_t *M = hex_to_bytes_le(M_chars[i], &M_len);
        if (!M) {
            printf("hex parse error for M%zu\n", i + 1);
            return 1;
        }

        printf("M%zu: ", i + 1);
        print_hex(M, M_len);

        streebog_make_hash(hash, M, M_len * 8);

        printf("H(M%zu): ", i + 1);
        print_hex(hash, 64);
        printf("\n");

        free(M);
    }

    /* === Через bignum_t (обёртка) === */
    printf("\n=== Via bignum wrapper ===\n");
    for (size_t i = 0; i < n; i++) {
        size_t M_len;
        uint8_t *M = hex_to_bytes_le(M_chars[i], &M_len);
        if (!M) {
            printf("hex parse error for M%zu\n", i + 1);
            return 1;
        }

        bignum_t bn_msg, bn_hash;
        bignum_init(&bn_msg);
        bignum_init(&bn_hash);

        int err = bignum_from_ui8_array(&bn_msg, M, M_len, false);
        free(M);
        if (err < 0) {
            printf("from_ui8_array failed: %d\n", err);
            bignum_delete(&bn_msg);
            bignum_delete(&bn_hash);
            return 1;
        }

        printf("M%zu (bignum): ", i + 1);
        bignum_print(&bn_msg);

        err = bignum_streebog_hash(&bn_hash, &bn_msg);
        if (err < 0) {
            printf("streebog_hash failed: %d\n", err);
            bignum_delete(&bn_msg);
            bignum_delete(&bn_hash);
            return 1;
        }

        printf("H(M%zu) (bignum): ", i + 1);
        bignum_print(&bn_hash);
        printf("\n");

        bignum_delete(&bn_msg);
        bignum_delete(&bn_hash);
    }

    return 0;
}