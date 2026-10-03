// hex.h
#ifndef HEX_H
#define HEX_H

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

/**
 * @brief Конвертирует hex-строку в malloc-нутый массив байт.
 *
 * @param hex Hex-строка (чётной длины).
 * @param out_len Указатель, куда записать длину массива в байтах.
 * @return Указатель на malloc-нутый массив байт в обратном порядке,
 *         или NULL при ошибке.
 *
 * @note Байты записываются в обратном порядке: последний символ
 *       hex-строки становится первым байтом массива.
 */
uint8_t *hex_to_bytes_le(const char *hex, size_t *out_len);

/**
 * @brief Печатает массив байт в hex.
 *
 * @param buf Указатель на массив байт.
 * @param len Количество байт.
 *
 * @note Байты печатаются в обратном порядке: от buf[len-1] до buf[0].
 */
void print_hex(const uint8_t *buf, size_t len);

#endif /* HEX_H */