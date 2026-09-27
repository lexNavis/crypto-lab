// hex.h
#ifndef HEX_H
#define HEX_H

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

/* Конвертирует hex-строку в malloc-нутый массив байт в обратном порядке.
 * Длину (в байтах) пишет в *out_len. Возвращает NULL при ошибке. */
uint8_t *hex_to_bytes_le(const char *hex, size_t *out_len);

/* Печатает массив байт в hex, начиная с последнего байта (buf[len-1]) и до buf[0]. */
void print_hex(const uint8_t *buf, size_t len);

#endif /* HEX_H */