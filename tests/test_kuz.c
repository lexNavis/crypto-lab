#include "../kuznyechik/kuznyechik.h"
#include "../bignum/bignum.h"
#include "../common/hex.h"
#include "args.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

int run_kuz_test(bool use_random_key, const char *custom_block)
{
  printf("------------------RUNNING KUZNYECHIK ALGORITHM------------------\n");
  kuz_key_t key;

  if (use_random_key)
  {
    kuz_keygen(&key);
  }
  else
  {
    size_t raw_len;
    uint8_t *raw = hex_to_bytes_le(kuz_K_char, &raw_len);
    if (!raw || raw_len != 32)
    {
      printf("kuz_K_char must be 32 bytes\n");
      free(raw);
      return 1;
    }
    kuz_keygen_from_raw(&key, raw);
    free(raw);
  }

  for (int i = 0; i < 10; i++)
  {
    printf("K%d = ", i + 1);
    print_hex(key.k_arr[i], 16);
  }

  const char *blocks[] = {kuz_M1_char, custom_block};
  size_t n = sizeof(blocks) / sizeof(blocks[0]);

  for (size_t i = 0; i < n; i++)
  {
    if (!blocks[i] || blocks[i][0] == '\0')
      continue;

    size_t block_len;
    uint8_t *block = hex_to_bytes_le(blocks[i], &block_len);
    if (!block)
    {
      printf("hex parse error for block %zu\n", i + 1);
      return 1;
    }
    if (block_len != 16)
    {
      printf("Block %zu must be 16 bytes, got %zu\n", i + 1, block_len);
      free(block);
      continue;
    }

    /* Конвертируем байты в bignum_t */
    bignum_t bn_block;
    bignum_init(&bn_block);
    int err = bignum_from_ui8_array(&bn_block, block, block_len, false);
    free(block);
    if (err < 0)
    {
      printf("from_ui8_array failed: %d\n", err);
      bignum_delete(&bn_block);
      return 1;
    }

    printf("Block     = ");
    bignum_print(&bn_block);

    /* Шифруем через обёртку */
    bignum_t bn_enc;
    bignum_init(&bn_enc);
    err = bignum_kuz_encrypt(&bn_enc, &bn_block, &key);
    if (err < 0)
    {
      printf("kuz_encrypt failed: %d\n", err);
      bignum_delete(&bn_block);
      bignum_delete(&bn_enc);
      return 1;
    }
    printf("Encrypted = ");
    bignum_print(&bn_enc);

    /* Дешифруем через обёртку */
    bignum_t bn_dec;
    bignum_init(&bn_dec);
    err = bignum_kuz_decrypt(&bn_dec, &bn_enc, &key);
    if (err < 0)
    {
      printf("kuz_decrypt failed: %d\n", err);
      bignum_delete(&bn_block);
      bignum_delete(&bn_enc);
      bignum_delete(&bn_dec);
      return 1;
    }
    printf("Decrypted = ");
    bignum_print(&bn_dec);

    /* Проверка: расшифрованное == исходное */
    if (bignum_cmp(&bn_block, &bn_dec) == 0)
    {
      printf("OK: decrypted == original\n");
    }
    else
    {
      printf("FAIL: decrypted != original\n");
    }

    bignum_delete(&bn_block);
    bignum_delete(&bn_enc);
    bignum_delete(&bn_dec);
    printf("\n");
  }

  return 0;
}