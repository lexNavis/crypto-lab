#include "hex.h"
#include <stdlib.h>
#include <string.h>

void print_hex(const uint8_t *buf, size_t len)
{
  for (int i = len - 1; i >= 0; i--)
  {
    printf("%02x", buf[i]);
  }
  printf("\n");
}

static int hexval(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

uint8_t *hex_to_bytes_le(const char *hex, size_t *out_len)
{
    size_t hex_len = strlen(hex);
    if (hex_len % 2 != 0)
        return NULL; // битая строка

    size_t n = hex_len / 2;
    uint8_t *out = malloc(n);
    if (!out)
        return NULL;

    for (size_t i = 0; i < n; i++)
    {
        int hi = hexval(hex[i * 2]);
        int lo = hexval(hex[i * 2 + 1]);
        if (hi < 0 || lo < 0)
        {
            free(out);
            return NULL;
        }
        out[n - 1 - i] = (uint8_t)((hi << 4) | lo);
    }

    *out_len = n;
    return out;
}