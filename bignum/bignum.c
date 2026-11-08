#include "bignum.h"
#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Обеспечивает needed число разрядов в числе val.
 * Если разрядов не хватает, расширяет его */
static int bignum_ensure_capacity(bignum_t *val, size_t needed) {
  if (val->capacity >= needed) {
    return 0;
  }
  /* Для новых чисел начальная выделяемая емкость - 1 ячейка */
  size_t new_cap = val->capacity ? val->capacity : 1;
  /* Удваиваем емкость, пока она не станет покрывать нужный размер */
  while (new_cap < needed) {
    new_cap *= 2;
  }
  uint32_t *new_val = realloc(val->val, new_cap * sizeof(uint32_t));
  if (!new_val) {
    return -ERR_REALLOC_FAILED;
  }
  val->val = new_val;
  val->capacity = new_cap;
  return 0;
}

/* Уменьшает разрядность числа val за счет игнорирования нулевых значащих
 * разрядов */
static void bignum_normalize(bignum_t *val) {
  while (val->length > 1 && val->val[val->length - 1] == 0) {
    val->length--;
  }
  if (bignum_is_zero(val)) {
    val->negative = false;
  }
}

/* Возвращает численное значение hex символа, если
 * таковое имеется */
static int hexval(const char chr) {
  if (chr >= '0' && chr <= '9') {
    return chr - '0';
  }
  if (chr >= 'a' && chr <= 'f') {
    return chr - 'a' + 10;
  }
  if (chr >= 'A' && chr <= 'F') {
    return chr - 'A' + 10;
  }
  return -1;
}
/* Добавляет число val в качестве нового старшего разряда числа dest */
static int bignum_push(bignum_t *dest, uint32_t val) {
  int err = bignum_ensure_capacity(dest, dest->length + 1);
  if (err < 0) {
    return err;
  }
  dest->val[dest->length] = val;
  dest->length++;
  return 0;
}
/* Беззнаковое сравнение двух чисел: 1 - первое число больше,
 * -1 - второе число больше, 0 - равенство чисел */
static int bignum_cmp_abs(const bignum_t *val_1, const bignum_t *val_2) {
  if (val_1->length > val_2->length)
    return 1;
  else if (val_1->length < val_2->length)
    return -1;
  else {
    for (size_t i = val_1->length; i > 0; i--) {
      if (val_1->val[i - 1] > val_2->val[i - 1])
        return 1;
      else if (val_1->val[i - 1] < val_2->val[i - 1])
        return -1;
    }
    return 0;
  }
}
/* Беззнаковое сложение чисел с записью результата в res */
static int bignum_add_abs(bignum_t *res, const bignum_t *val_1,
                          const bignum_t *val_2) {
  int err = 0;
  /* Сумма двух чисел не превышает разрядность наибольшего из них + 1
   * Доп разряд понадобится для помещения единицы переноса */
  size_t max_len;
  const bignum_t *max_val;
  if (val_1->length < val_2->length) {
    max_len = val_2->length;
    max_val = val_2; /* Сохраним для копирования без сложения */
  } else {
    max_len = val_1->length;
    max_val = val_1;
  }
  err = bignum_ensure_capacity(res, max_len + 1);
  if (err < 0) {
    return err;
  }
  size_t min_len =
      (val_1->length < val_2->length) ? val_1->length : val_2->length;
  uint32_t carry = 0;
  /* Основной цикл сложения: здесь младшая часть максимального числа
   * складывается со вторым числом */
  for (size_t i = 0; i < min_len; i++) {
    uint64_t sum = (uint64_t)val_1->val[i] + val_2->val[i] + carry;
    res->val[i] = (uint32_t)sum;
    carry = (uint32_t)(sum >> 32);
  }
  /* Цикл без сложения (но с учетом единицы переноса) */
  for (size_t i = min_len; i < max_len; i++) {
    uint64_t sum = (uint64_t)max_val->val[i] + carry;
    res->val[i] = (uint32_t)sum;
    carry = (uint32_t)(sum >> 32);
  }
  res->length = max_len;
  /* Если есть единица переноса из старшего разряда, добавим ее здесь */
  if (carry) {
    res->val[max_len] = carry;
    res->length++;
  }

  bignum_normalize(res);
  return 0;
}
/* Беззнаковое вычитание чисел с записью результата в res.
 * |a| >= |b| - должно гарантироваться для положительного результата
 * и корректного вычитания */
static int bignum_sub_abs(bignum_t *res, const bignum_t *val_1,
                          const bignum_t *val_2) {
  int err = bignum_ensure_capacity(res, val_1->length);
  if (err < 0) {
    return err;
  }
  int borrow = 0;
  /* Основной цикл вычитания */
  for (size_t i = 0; i < val_2->length; i++) {
    int64_t diff = (int64_t)val_1->val[i] - val_2->val[i] - borrow;
    if (diff < 0) {
      /* Если результат суммы отрицательный, берем по модулю 2^32 */
      res->val[i] = (uint32_t)(diff + (1LL << 32));
      borrow = 1;
    } else {
      res->val[i] = (uint32_t)(diff);
      borrow = 0;
    }
  }
  /* Переписываем старшую часть с учетом единицы занятости */
  for (size_t i = val_2->length; i < val_1->length; i++) {
    int64_t diff = (int64_t)val_1->val[i] - borrow;

    if (diff < 0) {
      /* Если результат суммы отрицательный, берем по модулю 2^32 */
      res->val[i] = (uint32_t)(diff + (1LL << 32));
      borrow = 1;
    } else {
      res->val[i] = (uint32_t)(diff);
      borrow = 0;
    }
  }
  res->negative = false;
  res->length = val_1->length;
  bignum_normalize(res);
  return 0;
}
/* Умножения числа bignum_t на единичный разряд с записью в res*/
int bignum_mul_digit(bignum_t *res, const bignum_t *val, uint32_t digit) {
  /* Результат умножение на "цифру" - возможное увеличение числа на разряд */
  bignum_t res_copy;
  bignum_init(&res_copy);
  int err = bignum_ensure_capacity(&res_copy, val->length + 1);
  if (err < 0) {
    bignum_delete(&res_copy);
    return err;
  }
  uint64_t carry = 0;
  for (size_t i = 0; i < val->length; i++) {
    uint64_t mul = (uint64_t)val->val[i] * digit + carry;
    res_copy.val[i] = (uint32_t)mul;
    carry = mul >> 32;
  }
  res_copy.length = val->length;
  /* Если в конце осталась единица переноса, добавим ее
   * и увеличим разрядность на 1 */
  if (carry) {
    res_copy.val[val->length] = carry;
    res_copy.length++;
  }
  res_copy.negative = false;
  bignum_normalize(&res_copy);
  err = bignum_copy(res, &res_copy);
  if (err < 0) {
    bignum_delete(&res_copy);
    return err;
  }
  bignum_delete(&res_copy);
  return 0;
}

/* Сдвиг влево на n разрядов */
int bignum_shift_left(bignum_t *a, size_t n) {
  if (n == 0)
    return 0;
  int err = bignum_ensure_capacity(a, a->length + n);
  if (err < 0)
    return err;
  memmove(a->val + n, a->val, a->length * sizeof(uint32_t));
  memset(a->val, 0, n * sizeof(uint32_t));
  a->length += n;
  return 0;
}

/* Сдвиг вправо на n разрядов */
void bignum_shift_right(bignum_t *a, size_t n) {
  if (n == 0)
    return;
  if (n >= a->length) {
    a->val[0] = 0;
    a->length = 1;
    a->negative = false;
    return;
  }
  memmove(a->val, a->val + n, (a->length - n) * sizeof(uint32_t));
  a->length -= n;
  /* Уберем лишние нулевые значащие разряды */
  bignum_normalize(a);
}

/* Обнуление числа val */
int bignum_set_zero(bignum_t *val) {
  int err = bignum_ensure_capacity(val, 1);
  if (err < 0)
    return err;
  val->val[0] = 0;
  val->length = 1;
  val->negative = false;
  return 0;
}

/* Сдвиг влево на n бит */
int bignum_shift_left_bits(bignum_t *a, size_t n) {
  size_t word_count = n / 32;
  size_t bit_count = n % 32;
  /* Сначала сдвинем биты. Величина битового сдвига
   * не больше слова, поэтому вектор может увеличиться
   * максимум на 1 разряд. */
  int err = bignum_ensure_capacity(a, a->length + word_count + 1);
  if (err < 0) {
    return err;
  }
  uint32_t carry = 0;
  if (bit_count > 0) {
    for (size_t i = 0; i < a->length; i++) {
      /* Сохраним исходное число */
      uint32_t cur = a->val[i];
      /* Заполним освободившиеся биты прошлым переносом после сдвига */
      a->val[i] = (a->val[i] << bit_count) | carry;
      /* Вычислим новый перенос, который будет сдвинут в левый разряд */
      carry = (cur >> (32 - bit_count));
    }
    if (carry != 0) {
      a->val[a->length] = carry;
      a->length++;
    }
  }
  /* Теперь слова */
  err = bignum_shift_left(a, word_count);
  return err;
}

/* Сдвиг вправо на n бит */
void bignum_shift_right_bits(bignum_t *a, size_t n) {
  size_t word_count = n / 32;
  size_t bit_count = n % 32;
  uint32_t carry = 0;
  if (bit_count > 0) {
    for (size_t i = a->length; i > 0; i--) {
      /* Сохраним исходное число */
      uint32_t cur = a->val[i - 1];
      /* Заполним старшие биты прошлым переносом после сдвига */
      a->val[i - 1] = (a->val[i - 1] >> bit_count) | carry;
      /* Вычислим новый перенос, который будет сдвинут в правый разряд */
      carry = (cur << (32 - bit_count));
    }
  }
  /* Теперь слова */
  bignum_shift_right(a, word_count);
  bignum_normalize(a);
}

/* Получение числа значащих нулей в начале числа х */
static int bignum_leading_zeros(uint32_t x) {
  if (x == 0)
    return 32;
  return __builtin_clz(x);
}

/* Получение номера старшего значащего разряда(размером bits) в word */
static size_t bignum_top_digit(uint32_t word, size_t bits) {
  size_t count = 32 / bits; /* сколько разрядов в слове */
  for (size_t i = count; i > 0; i--) {
    if ((word >> ((i - 1) * bits)) & ((1u << bits) - 1)) {
      return i - 1;
    }
  }
  return 0;
}

void bignum_init(bignum_t *val) {
  val->capacity = 0;
  val->length = 0;
  val->negative = false;
  /* init не иницализирует число */
  val->val = NULL;
}

void bignum_delete(bignum_t *val) {
  free(val->val);
  val->val = NULL;
  val->capacity = 0;
  val->length = 0;
  val->negative = false;
}

int bignum_copy(bignum_t *dest, const bignum_t *src) {
  if (!dest || !src)
    return -ERR_NULLPTR;
  if (dest == src)
    return 0;
  int err = bignum_ensure_capacity(dest, src->length);
  if (err < 0) {
    return err;
  }
  for (size_t i = 0; i < src->length; i++) {
    dest->val[i] = src->val[i];
  }
  dest->negative = src->negative;
  dest->length = src->length;
  bignum_normalize(dest);
  return 0;
}

void bignum_print(const bignum_t *val) {
  if (val->negative)
    printf("-");
  for (size_t i = val->length; i > 0; i--) {
    printf("%08x", val->val[i - 1]);
  }
  printf("\n");
}

void bignum_print_dec(const bignum_t *val) {
  if (bignum_is_zero(val)) {
    printf("0");
    return;
  }
  if (val->negative) {
    printf("-");
  }
  /* Копия, потому что будем делить. */
  bignum_t tmp;
  bignum_init(&tmp);
  bignum_copy(&tmp, val);
  tmp.negative = false;
  /* Собираем остатки от деления на 10^9. */
  uint32_t base = 1000000000;
  uint32_t *parts = NULL;
  size_t count = 0;
  size_t cap = 0;

  bignum_t divisor, rem;
  bignum_init(&divisor);
  bignum_init(&rem);
  bignum_from_ui32_array(&divisor, &base, 1, false);

  while (!bignum_is_zero(&tmp)) {
    bignum_mod(&rem, &tmp, &divisor);
    bignum_div(&tmp, &tmp, &divisor);
    /* rem < 10^9, значит rem->val[0] содержит остаток. */
    if (count == cap) {
      cap = cap ? cap * 2 : 4;
      parts = realloc(parts, cap * sizeof(uint32_t));
    }
    parts[count++] = rem.val[0];
  }

  /* Печатаем старшую часть без ведущих нулей, остальные — с ведущими. */
  printf("%u", parts[count - 1]);
  for (size_t i = count - 1; i > 0; i--) {
    printf("%09u", parts[i - 1]);
  }

  free(parts);
  bignum_delete(&tmp);
  bignum_delete(&divisor);
  bignum_delete(&rem);
}

int bignum_from_ui8_array(bignum_t *dest, const uint8_t *src, size_t size,
                          bool neg) {
  int err;
  if (!dest || !src) {
    return -ERR_NULLPTR;
  }
  if (size == 0) {
    return -ERR_INVALID_SIZE;
  }
  size_t word_count = (size + 3) / 4;
  err = bignum_ensure_capacity(dest, word_count);
  if (err != 0) {
    return err;
  }
  for (size_t i = 0; i < word_count; i++) {
    /* Так как word_count округлен до большего числа,
     * то максимальный элемент в src может быть на отрезке
     * [4*i, 4*i+3] в зависимости от округления. Для предотвращения
     * segfault вводятся проверки для каждого разряда */
    uint32_t b0 = (4 * i < size) ? src[4 * i] : 0;
    uint32_t b1 = (4 * i + 1 < size) ? src[4 * i + 1] : 0;
    uint32_t b2 = (4 * i + 2 < size) ? src[4 * i + 2] : 0;
    uint32_t b3 = (4 * i + 3 < size) ? src[4 * i + 3] : 0;
    dest->val[i] = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
  }
  dest->negative = neg;
  dest->length = word_count;
  bignum_normalize(dest);
  return 0;
}

int bignum_from_ui32_array(bignum_t *dest, const uint32_t *src, size_t size,
                           bool neg) {
  int err;
  if (!dest || !src) {
    return -ERR_NULLPTR;
  }
  if (size == 0) {
    return -ERR_INVALID_SIZE;
  }
  err = bignum_ensure_capacity(dest, size);
  if (err != 0) {
    return err;
  }
  for (size_t i = 0; i < size; i++) {
    dest->val[i] = src[i];
  }
  dest->negative = neg;
  dest->length = size;
  bignum_normalize(dest);
  return 0;
}

int bignum_from_hex_string(bignum_t *dest, const char *hex, bool neg) {
  if (!dest || !hex)
    return -ERR_NULLPTR;
  size_t n = strlen(hex);
  if (n == 0)
    return -ERR_INVALID_SIZE;

  dest->length = 0;
  uint32_t digit = 0;
  int shift = 0;

  for (size_t i = 0; i < n; i++) {
    int byte = hexval(hex[n - 1 - i]);
    if (byte < 0) {
      dest->length = 0;
      dest->negative = false;
      return -ERR_INVALID_STRING;
    }
    /* Символ занимает 4 бита, shift * 4 определяет 1 из 8 позиций в digit */
    digit |= (uint32_t)byte << (shift * 4);
    shift++;
    if (shift == 8) {
      int err = bignum_push(dest, digit);
      if (err < 0) {
        dest->length = 0;
        dest->negative = false;
        return err;
      }
      shift = 0;
      digit = 0;
    }
  }

  if (shift > 0) {
    int err = bignum_push(dest, digit);
    if (err < 0) {
      dest->length = 0;
      dest->negative = false;
      return err;
    }
  }

  dest->negative = neg;
  bignum_normalize(dest);
  return 0;
}

int bignum_from_gmp(bignum_t *dest, const mpz_t src) {
  if (!dest) {
    return -ERR_NULLPTR;
  }
  int err = 0;
  int sign = mpz_sgn(src);
  if (sign == 0) {
    return bignum_set_zero(dest);
  }
  size_t count = 0;
  /* Первый вызов нужен для получения длины числа mpz_t */
  mpz_export(NULL, &count, -1, sizeof(uint32_t), 0, 0, src);
  err = bignum_ensure_capacity(dest, count);
  if (err < 0)
    return err;
  mpz_export(dest->val, NULL, -1, sizeof(uint32_t), 0, 0, src);
  dest->length = count;
  dest->negative = (sign < 0);
  return 0;
}

int bignum_rand(bignum_t *res, size_t bits) {
  if (!res || bits == 0)
    return -ERR_INVALID_PARAM;

  size_t bytes = (bits + 7) / 8;
  uint8_t *buf = malloc(bytes);
  if (!buf)
    return -ERR_MALLOC_FAILED;

  int err = read_random_bytes(buf, bytes);
  if (err < 0) {
    free(buf);
    return -ERR_GET_RANDOM_FAIL_TOTAL;
  }

  /* Обрезать старшие биты, чтобы число было ровно bits бит */
  size_t extra = bytes * 8 - bits;
  if (extra > 0)
    buf[bytes - 1] &= (uint8_t)(0xFF >> extra);

  err = bignum_from_ui8_array(res, buf, bytes, false);
  free(buf);
  return err;
}

int bignum_rand_range(bignum_t *res, const bignum_t *max) {
  if (!res || !max)
    return -ERR_NULLPTR;
  if (bignum_is_zero(max))
    return -ERR_INVALID_PARAM;
  size_t bits = bignum_length(max) * 32; // верхняя оценка
  int err = bignum_rand(res, bits);
  if (err < 0)
    return err;
  return bignum_mod(res, res, max);
}

long bignum_to_ui8_array(uint8_t *dest, size_t dest_size, const bignum_t *src) {
  if (!dest || !src) {
    return -ERR_NULLPTR;
  }

  if (src->length == 0) {
    if (dest_size < 1)
      return -ERR_INVALID_SIZE;
    dest[0] = 0;
    return 1;
  }

  size_t last_word = src->length - 1; // после normalize старшее слово ненулевое
  uint32_t top = src->val[last_word];
  /* Определим точное число байт, которое необходимо записать.
   * Для этого определим, сколько значащих байт есть в последнем разряде
   * Точное число байт определяется как число полностью заполненных разрядов
   * + число значащих байт в последнем разряде */
  size_t last_byte = bignum_top_digit(top, 8);
  size_t total = last_word * 4 + last_byte + 1;

  if (dest_size < total) {
    return -ERR_INVALID_SIZE;
  }

  for (size_t i = 0; i < total; i++) {
    dest[i] = (src->val[i / 4] >> ((i % 4) * 8)) & 0xFF;
  }
  return total;
}

long bignum_to_ui32_array(uint32_t *dest, size_t dest_size,
                          const bignum_t *src) {
  if (!dest || !src) {
    return -ERR_NULLPTR;
  }

  if (src->length == 0) {
    if (dest_size < 1)
      return -ERR_INVALID_SIZE;
    dest[0] = 0;
    return 1;
  }
  if (dest_size < src->length) {
    return -ERR_INVALID_SIZE;
  }
  memcpy(dest, src->val, src->length * sizeof(uint32_t));
  return (long)src->length;
}

long bignum_to_hex_string(char *dest, size_t dest_size, const bignum_t *src) {
  if (!dest || !src)
    return -ERR_NULLPTR;

  if (src->length == 0) {
    if (dest_size < 2)
      return -ERR_INVALID_SIZE;
    dest[0] = '0';
    dest[1] = '\0';
    return 1;
  }

  size_t last_word = src->length - 1;
  uint32_t top = src->val[last_word];
  /* Число разрядов на 1 больше номера старшего разряда в числе */
  size_t top_nibble = bignum_top_digit(top, 4) + 1;
  /* Длину считаем с учетом нуль-терминатора */
  size_t len = last_word * 8 + top_nibble + 1;
  if (dest_size < len)
    return -ERR_INVALID_SIZE;

  size_t off = 0;
  /* Старшее слово — без ведущих нулей. */
  off += snprintf(dest + off, dest_size - off, "%x", src->val[last_word]);
  /* Остальные — с ведущими нулями. */
  for (size_t i = last_word; i > 0; i--) {
    off += snprintf(dest + off, dest_size - off, "%08x", src->val[i - 1]);
  }
  dest[off] = '\0';
  return (long)off;
}

void bignum_to_gmp(mpz_t dest, const bignum_t *src) {
  mpz_import(dest, src->length, -1, sizeof(uint32_t), 0, 0, src->val);
  if (src->negative) {
    mpz_neg(dest, dest);
  }
}

size_t bignum_length(const bignum_t *val) {
  if (val) {
    return val->length;
  }
  return 0;
}
size_t bignum_bit_length(const bignum_t *val) {
  if (!val || val->length == 0)
    return 0;
  /* Старший разряд не равен нулю после normalize */
  return (val->length - 1) * 32 +
         (32 - bignum_leading_zeros(val->val[val->length - 1]));
}
size_t bignum_capacity(const bignum_t *val) {
  if (val) {
    return val->capacity;
  }
  return 0;
}

bool bignum_is_zero(const bignum_t *val) {
  if (val) {
    return (val->length == 1 && val->val[0] == 0);
  }
  return false;
}
bool bignum_is_even(const bignum_t *val) {
  if (val) {
    return (val->length > 0 && (val->val[0] & 1) == 0);
  }
  return false;
}

bool bignum_is_negative(const bignum_t *val) {
  if (val) {
    return val->negative;
  }
  return false;
}

void bignum_set_negative(bignum_t *val, bool negative) {
  if (val) {
    val->negative = negative;
  }
}

bool bignum_bit(const bignum_t *val, size_t pos) {
  if (!val) {
    return false;
  }
  size_t word = pos / 32;
  if (word >= val->length)
    return false;
  return (val->val[word] >> (pos % 32)) & 1;
}

int bignum_cmp(const bignum_t *val_1, const bignum_t *val_2) {
  if (val_1->negative != val_2->negative) {
    return val_1->negative ? -1 : 1;
  }

  int cmp = bignum_cmp_abs(val_1, val_2);

  /* Если оба отрицательные — инвертируем */
  if (val_1->negative) {
    cmp = -cmp;
  }
  return cmp;
}

int bignum_add(bignum_t *res, const bignum_t *val_1, const bignum_t *val_2) {
  if (!res || !val_1 || !val_2) {
    return -ERR_NULLPTR;
  }
  int err = 0;
  /* Проверка на алиасинг (запись результата на место операнда) */
  bignum_t val_1_copy, val_2_copy;
  bignum_init(&val_1_copy);
  bignum_init(&val_2_copy);
  const bignum_t *a = val_1;
  const bignum_t *b = val_2;
  /* Если указатели совпадают, то заменяем оригинал операнда на копию */
  if (res == val_1) {
    err = bignum_copy(&val_1_copy, val_1);
    if (err < 0)
      goto cleanup;
    a = &val_1_copy;
  }
  if (res == val_2) {
    err = bignum_copy(&val_2_copy, val_2);
    if (err < 0)
      goto cleanup;
    b = &val_2_copy;
  }
  /* Сравнение знаков операндов */
  if (a->negative == b->negative) {
    /* Для одного знака производится сложение по модулю,
     * а знак берется у любого операнда */
    err = bignum_add_abs(res, a, b);
    if (err < 0) {
      goto cleanup;
    }
    res->negative = a->negative;
  } else {
    /* Для разных знаков производится вычитание меньшего модуля
     * из большего, а знак берется у большего по модулю числа */
    int cmp = bignum_cmp_abs(a, b);
    if (cmp >= 0) {
      err = bignum_sub_abs(res, a, b);
      if (err < 0) {
        goto cleanup;
      }
      res->negative = a->negative;
    } else {
      err = bignum_sub_abs(res, b, a);
      if (err < 0) {
        goto cleanup;
      }
      res->negative = b->negative;
    }
  }
  bignum_normalize(res);

cleanup:
  bignum_delete(&val_1_copy);
  bignum_delete(&val_2_copy);
  return err;
}

int bignum_add_digit(bignum_t *res, const bignum_t *val, uint32_t digit) {
  if (!res || !val)
    return -ERR_NULLPTR;

  int err = bignum_ensure_capacity(res, val->length + 1);
  if (err < 0)
    return err;

  uint64_t carry = digit;
  for (size_t i = 0; i < val->length; i++) {
    uint64_t sum = (uint64_t)val->val[i] + carry;
    res->val[i] = (uint32_t)sum;
    carry = sum >> 32;
  }
  res->length = val->length;
  if (carry) {
    res->val[res->length] = (uint32_t)carry;
    res->length++;
  }
  res->negative = false;
  bignum_normalize(res);
  return 0;
}

int bignum_sub(bignum_t *res, const bignum_t *val_1, const bignum_t *val_2) {
  if (!res || !val_1 || !val_2) {
    return -ERR_NULLPTR;
  }
  int err = 0;
  /* Проверка на алиасинг (запись результата на место операнда) */
  bignum_t val_1_copy, val_2_copy;
  bignum_init(&val_1_copy);
  bignum_init(&val_2_copy);
  const bignum_t *a = val_1;
  const bignum_t *b = val_2;
  /* Если указатели совпадают, то заменяем оригинал операнда на копию */
  if (res == val_1) {
    err = bignum_copy(&val_1_copy, val_1);
    if (err < 0)
      goto cleanup;
    a = &val_1_copy;
  }
  if (res == val_2) {
    err = bignum_copy(&val_2_copy, val_2);
    if (err < 0)
      goto cleanup;
    b = &val_2_copy;
  }
  /* Сравнение знаков операндов */
  if (a->negative != b->negative) {
    /* Для разных знаков производится сложение модулей,
     * а знак берется у первого числа */
    err = bignum_add_abs(res, a, b);
    if (err < 0) {
      goto cleanup;
    }
    res->negative = a->negative;
  } else {
    /* Для одного знака производится вычитание модулей,
     * а знак берется у наибольшего по модулю числа по правилу:
     * знак первого числа или инвертированный знак второго числа */
    int cmp = bignum_cmp_abs(a, b);
    if (cmp >= 0) {
      err = bignum_sub_abs(res, a, b);
      if (err < 0) {
        goto cleanup;
      }
      res->negative = a->negative;
    } else {
      err = bignum_sub_abs(res, b, a);
      if (err < 0) {
        goto cleanup;
      }
      res->negative = !b->negative;
    }
  }
  bignum_normalize(res);
cleanup:
  bignum_delete(&val_1_copy);
  bignum_delete(&val_2_copy);
  return err;
}

int bignum_sub_digit(bignum_t *res, const bignum_t *val, uint32_t digit) {
  if (!res || !val)
    return -ERR_NULLPTR;
  if (val->length == 1 && val->val[0] < digit)
    return -ERR_INVALID_PARAM; // Отрицательный параметр

  int err = bignum_ensure_capacity(res, val->length);
  if (err < 0)
    return err;

  int64_t borrow = digit;
  for (size_t i = 0; i < val->length; i++) {
    int64_t diff = (int64_t)val->val[i] - borrow;
    if (diff < 0) {
      res->val[i] = (uint32_t)(diff + ((int64_t)1 << 32));
      borrow = 1;
    } else {
      res->val[i] = (uint32_t)diff;
      borrow = 0;
    }
  }
  res->length = val->length;
  res->negative = false;
  bignum_normalize(res);
  return 0;
}

int bignum_mul(bignum_t *res, const bignum_t *val_1, const bignum_t *val_2) {
  if (!res || !val_1 || !val_2) {
    return -ERR_NULLPTR;
  }
  bignum_t res_copy;
  bignum_init(&res_copy);
  /* Результат промежуточных умножений */
  bignum_t mul;
  bignum_init(&mul);
  int err = bignum_ensure_capacity(&res_copy, val_1->length + val_2->length);
  if (err < 0) {
    goto delete_all;
  }
  /* Лучше в качестве множителя выбрать число с наименьшим числом разрядов */
  if (bignum_cmp_abs(val_1, val_2) == -1) {
    /* Если второе число больше, поменяем их локально местами */
    const bignum_t *tmp = val_1;
    val_1 = val_2;
    val_2 = tmp;
  }

  /* После сравнения гарантируется, что второе число - минимальное.
   * Обычное умножение подразумевает умножение первого числа на очередной
   * разряд второго, сдвига результата на число, равное индексу этого разряда
   * и сложение с результатом */
  for (size_t i = 0; i < val_2->length; i++) {
    err = bignum_mul_digit(&mul, val_1, val_2->val[i]);
    if (err < 0) {
      goto delete_all;
    }
    err = bignum_shift_left(&mul, i);
    if (err < 0) {
      goto delete_all;
    }
    err = bignum_add_abs(&res_copy, &res_copy, &mul);
    if (err < 0) {
      goto delete_all;
    }
  }
  res_copy.negative = val_1->negative ^ val_2->negative;
  bignum_normalize(&res_copy);
  err = bignum_copy(res, &res_copy);

delete_all:
  bignum_delete(&mul);
  bignum_delete(&res_copy);
  return err;
}

int bignum_divmod(bignum_t *quot, bignum_t *rem, const bignum_t *val_1,
                  const bignum_t *val_2) {
  if (!quot || !rem || !val_1 || !val_2) {
    return -ERR_NULLPTR;
  }
  if (quot == rem) {
    return -ERR_INVALID_PARAM;
  }
  /* Деление на ноль */
  if (bignum_is_zero(val_2)) {
    return -ERR_DIVISION_BY_ZERO;
  }
  int err = 0;
  /* Локальные копии для сохранности входных параметров */
  bignum_t a;
  bignum_t b;
  bignum_init(&a);
  bignum_init(&b);
  err = bignum_copy(&a, val_1);
  if (err < 0) {
    goto cleanup;
  }
  err = bignum_copy(&b, val_2);
  if (err < 0) {
    goto cleanup;
  }
  /* Тривиальные случаи */
  int cmp = bignum_cmp_abs(&a, &b);
  /* val_1 < val_2 по модулю */
  if (cmp == -1) {
    err = bignum_copy(rem, &a); // сначала rem
    if (err < 0) {
      goto cleanup;
    }
    err = bignum_set_zero(quot); // потом quot
    if (err < 0) {
      goto cleanup;
    }
    goto cleanup;
  }
  /* val_1 = val_2 по модулю */
  if (cmp == 0) {
    uint32_t one = 1;
    err = bignum_from_ui32_array(quot, &one, 1, a.negative ^ b.negative);
    if (err < 0) {
      goto cleanup;
    }
    err = bignum_set_zero(rem);
    if (err < 0) {
      goto cleanup;
    }
    goto cleanup;
  }
  /* Основной цикл деления, реализованный на основе алгоритма Кнута */
  /* Определим величину сдвига, чтобы старший бит старшей части делителя стал 1
   */

  int shift = bignum_leading_zeros(val_2->val[val_2->length - 1]);
  err = bignum_shift_left_bits(&b, shift);
  if (err < 0) {
    goto cleanup;
  }
  /* Для алгоритма критично появление у делимого нового разряда,
   * поэтому добавим его вручную */
  err = bignum_push(&a, 0);
  if (err < 0) {
    goto cleanup;
  }
  err = bignum_shift_left_bits(&a, shift);
  if (err < 0) {
    goto cleanup;
  }

  size_t dr_len = b.length; // divisor
  /* Окно алгоритма — n+1 слов, где n = dr_len (число разрядов делителя).
   * Поэтому последняя позиция окна: m = length(a) - n - 1. */
  size_t m = a.length - b.length - 1;
  err = bignum_ensure_capacity(quot, m + 1);
  if (err < 0) {
    goto cleanup;
  }
  quot->length = m + 1;
  /* В рамках каждой итерации цикла рассматривается
   * фрагмент делимого [j..(j + dr_len)]*/
  for (size_t i = m + 1; i > 0; i--) {
    size_t j = i - 1;
    /* Поделим два старших разряда делимого на старший разряд
     * нормализованного делителя аппаратными средствами */
    uint64_t a_high =
        ((uint64_t)a.val[j + dr_len] << 32) | (uint64_t)a.val[j + dr_len - 1];
    uint64_t b_high = b.val[dr_len - 1];
    uint64_t result;
    uint64_t rest;
    bool is_overflow = false;
    /* Проверка, вызовет ли результат переполнение разрядной сетки*/
    if (a.val[j + dr_len] >= b.val[dr_len - 1]) {
      /* В таком случае устанавливаем максимальное
       * число для частного */
      result = 0xFFFFFFFF;
      rest = 0;
      is_overflow = true;
    } else {
      result = a_high / b_high;
      rest = a_high % b_high;
    }
    /* Мы нашли частное для старшей части фрагмента.
     * Осталось обеспечить, чтобы частное подходило
     * для младшей части. (Только если нет переполнения)*/
    if (dr_len > 1 && !is_overflow) {
      /* Нужно обеспечить, чтобы произведение частного
       * на младшую часть делителя было не больше чем остаток
       * фрагмента делимого. Иначе разность будет отрицательная.
       * (При вычитании младшая часть займет 1 у старшей и последняя станет
       * меньше старшей части произведения) */
      while (result * b.val[dr_len - 2] >
             (rest << 32) + (uint64_t)a.val[j + dr_len - 2]) {
        /* Уменьшаем частное и возвращаем остатку множители */
        result--;
        rest += b_high;
        /* Если остаток превысил 32 бита, можно досрочно выйти из цикла */
        if (rest >= (1ULL << 32)) {
          break;
        }
      }
    }
    /* Пробное вычитание - очередная проверка на корректность частного */
    bignum_t window;
    bignum_init(&window);

    bignum_from_ui32_array(&window, a.val + j, dr_len + 1, false);
    bignum_t prod;
    bignum_init(&prod);
    bignum_mul_digit(&prod, &b, result);
    /* Если окно меньше произведения — result слишком большой.
     * Уменьшаем result на 1 и возвращаем одно b обратно в остаток числа окна.
     */
    if (bignum_cmp_abs(&window, &prod) == -1) {
      result--;
      bignum_add(&window, &window, &b);
    }

    /* Теперь вычитаем. window = window - prod. */
    bignum_sub_abs(&window, &window, &prod);
    /* Записываем window обратно в u[j..j+dr_len]. */
    for (size_t k = 0; k < dr_len + 1; k++) {
      if (k < window.length) {
        a.val[j + k] = window.val[k];
      } else {
        a.val[j + k] = 0;
      }
    }

    /* Записываем очередное слово частного. */
    quot->val[j] = result;

    bignum_delete(&window);
    bignum_delete(&prod);
  }

  /* quot уже заполнен в цикле. Установим длину и нормализуем. */
  quot->length = m + 1; /* m = a.length - b.length (до цикла) */
  quot->negative = val_1->negative ^ val_2->negative;
  bignum_normalize(quot);

  /* Остаток — это a.val[0..dr_len-1], сдвинутый вправо на shift. */
  err = bignum_from_ui32_array(rem, a.val, dr_len, false);
  if (err < 0) {
    goto cleanup;
  }
  /* Денормализация: сдвиг вправо на shift. */
  bignum_shift_right_bits(rem, shift);
  rem->negative = val_1->negative;
  bignum_normalize(rem);

cleanup:
  bignum_delete(&a);
  bignum_delete(&b);
  return err;
}

/* Использует divmod, но остаток игнорирует */
int bignum_div(bignum_t *quot, const bignum_t *val_1, const bignum_t *val_2) {
  bignum_t rem;
  bignum_init(&rem);
  int err = bignum_divmod(quot, &rem, val_1, val_2);
  bignum_delete(&rem);
  return err;
}
/* Использует divmod, но целую часть игнорирует */
int bignum_mod(bignum_t *rem, const bignum_t *val_1, const bignum_t *val_2) {
  bignum_t quot;
  bignum_init(&quot);
  int err = bignum_divmod(&quot, rem, val_1, val_2);
  bignum_delete(&quot);
  return err;
}

int bignum_modinv(bignum_t *res, const bignum_t *a, const bignum_t *m)
{
  if (!res || !a || !m)
    return -ERR_NULLPTR;

  /* Модуль должен быть > 1, иначе обратный элемент не определён
   * (mod 1 всё ≡ 0, mod 0 — деление на ноль). */
  bignum_t one;
  bignum_init(&one);
  int err = bignum_from_ui32_array(&one, (uint32_t[]){1}, 1, false);
  if (err < 0)
  {
    bignum_delete(&one);
    return err;
  }
  if (bignum_cmp(m, &one) != 1)
  {
    bignum_delete(&one);
    return -ERR_INVALID_PARAM;
  }

  /* Приводим a к положительному остатку [0, m-1], чтобы дальше
   * работать только с неотрицательными числами. Если a отрицательное,
   * bignum_mod даст отрицательный остаток (знак сохраняется как у
   * делимого), поэтому добавляем m. */
  bignum_t a_mod;
  bignum_init(&a_mod);
  err = bignum_mod(&a_mod, a, m);
  if (err < 0)
    goto cleanup;
  if (a_mod.negative)
  {
    err = bignum_add(&a_mod, &a_mod, m);
    if (err < 0)
      goto cleanup;
  }

  /* Расширенный алгоритм Евклида.
   *
   * Идея: поддерживаем инвариант
   *     old_r = a * old_s + m * old_t
   *     r     = a * s     + m * t
   * На каждом шаге делаем шаг Евклида (r_new = old_r mod r) и
   * соответственно обновляем коэффициенты s, t. Когда r = 0,
   * old_r = gcd(a, m), а old_s — коэффициент при a. Если gcd = 1,
   * то old_s — искомый обратный элемент (a * old_s ≡ 1 mod m).
   *
   * t нам не нужен для ответа, но он нужен для инварианта при
   * обновлении s (через t нельзя выразить s без t). Поэтому
   * поддерживаем и t, хотя в конце он не используется. */
  bignum_t old_r, r, old_s, s, tmp, q, prod;
  bignum_init(&old_r);
  bignum_init(&r);
  bignum_init(&old_s);
  bignum_init(&s);
  bignum_init(&tmp);
  bignum_init(&q);
  bignum_init(&prod);

  /* old_r = m, r = a_mod */
  err = bignum_copy(&old_r, m);
  if (err < 0) goto cleanup2;
  err = bignum_copy(&r, &a_mod);
  if (err < 0) goto cleanup2;

  /* old_s = 0, s = 1 (коэффициенты при a) */
  err = bignum_set_zero(&old_s);
  if (err < 0) goto cleanup2;
  err = bignum_copy(&s, &one);
  if (err < 0) goto cleanup2;

  while (!bignum_is_zero(&r))
  {
    /* q = old_r / r (целая часть) */
    err = bignum_div(&q, &old_r, &r);
    if (err < 0) goto cleanup2;

    /* (old_r, r) = (r, old_r - q*r) */
    err = bignum_mul(&prod, &q, &r);
    if (err < 0) goto cleanup2;
    err = bignum_sub(&tmp, &old_r, &prod);
    if (err < 0) goto cleanup2;
    err = bignum_copy(&old_r, &r);
    if (err < 0) goto cleanup2;
    err = bignum_copy(&r, &tmp);
    if (err < 0) goto cleanup2;

    /* (old_s, s) = (s, old_s - q*s) */
    err = bignum_mul(&prod, &q, &s);
    if (err < 0) goto cleanup2;
    err = bignum_sub(&tmp, &old_s, &prod);
    if (err < 0) goto cleanup2;
    err = bignum_copy(&old_s, &s);
    if (err < 0) goto cleanup2;
    err = bignum_copy(&s, &tmp);
    if (err < 0) goto cleanup2;
  }

  /* Теперь old_r = gcd(a, m). Если != 1 — обратного элемента нет. */
  if (bignum_cmp(&old_r, &one) != 0)
  {
    err = -ERR_INVALID_PARAM;
    goto cleanup2;
  }

  /* old_s — коэффициент при a: a * old_s ≡ 1 (mod m).
   * Приводим к [0, m-1]: old_s может быть отрицательным
   * (bignum_mod сохраняет знак делимого), тогда добавляем m. */
  err = bignum_mod(res, &old_s, m);
  if (err < 0) goto cleanup2;
  if (res->negative)
  {
    err = bignum_add(res, res, m);
    if (err < 0) goto cleanup2;
  }

  err = 0;

cleanup2:
  bignum_delete(&old_r);
  bignum_delete(&r);
  bignum_delete(&old_s);
  bignum_delete(&s);
  bignum_delete(&tmp);
  bignum_delete(&q);
  bignum_delete(&prod);
cleanup:
  bignum_delete(&a_mod);
  bignum_delete(&one);
  return err;
}

int bignum_pow_mod(bignum_t *res, const bignum_t *base, const bignum_t *exp,
                   const bignum_t *mod) {
  if (!res || !base || !exp || !mod) {
    return -ERR_NULLPTR;
  }
  int err = 0;
  bignum_t res_copy;
  bignum_init(&res_copy);
  /* Установим начальное значение 1 для результата */
  uint32_t one = 1;
  err = bignum_from_ui32_array(&res_copy, &one, 1, false);
  if (err < 0) {
    goto delete_from_res_copy;
  }
  /* Копии изменяемых величин для сохранения исходников */
  bignum_t base_copy;
  bignum_init(&base_copy);
  err = bignum_copy(&base_copy, base);
  if (err < 0) {
    goto delete_from_base_copy;
  }
  bignum_t exp_copy;
  bignum_init(&exp_copy);
  err = bignum_copy(&exp_copy, exp);
  if (err < 0) {
    goto delete_from_exp_copy;
  }
  bignum_t zero;
  bignum_init(&zero);
  err = bignum_set_zero(&zero);
  if (err < 0) {
    goto delete_all;
  }
  err = bignum_mod(&base_copy, &base_copy, mod);
  if (err < 0) {
    goto delete_all;
  }
  /* Цикл по разрядам степени. Продолжаем, пока остались
   * необраюотанные разряды */
  while (bignum_cmp_abs(&exp_copy, &zero) == 1) {
    /* Если очередной разряд степени 1, тогда умножаем результат
     * на это умножение. Иначе копим дальше. */
    if (!bignum_is_even(&exp_copy)) {
      err = bignum_mul(&res_copy, &res_copy, &base_copy);
      if (err < 0) {
        goto delete_all;
      }
      err = bignum_mod(&res_copy, &res_copy, mod);
      if (err < 0) {
        goto delete_all;
      }
    }
    /* Возводим основание в квадрат по модулю mod */
    err = bignum_mul(&base_copy, &base_copy, &base_copy);
    if (err < 0) {
      goto delete_all;
    }
    err = bignum_mod(&base_copy, &base_copy, mod);
    if (err < 0) {
      goto delete_all;
    }
    bignum_shift_right_bits(&exp_copy, 1);
  }
  err = bignum_copy(res, &res_copy);
  if (err < 0) {
    goto delete_all;
  }
delete_all:
  bignum_delete(&zero);
delete_from_exp_copy:
  bignum_delete(&exp_copy);
delete_from_base_copy:
  bignum_delete(&base_copy);
delete_from_res_copy:
  bignum_delete(&res_copy);
  return err;
}

int bignum_pow(bignum_t *res, const bignum_t *base, const bignum_t *exp) {
  if (!res || !base || !exp) {
    return -ERR_NULLPTR;
  }
  int err = 0;
  bignum_t res_copy;
  bignum_init(&res_copy);
  /* Установим начальное значение 1 для результата */
  uint32_t one = 1;
  err = bignum_from_ui32_array(&res_copy, &one, 1, false);
  if (err < 0) {
    goto delete_from_res_copy;
  }
  /* Копии изменяемых величин для сохранения исходников */
  bignum_t base_copy;
  bignum_init(&base_copy);
  err = bignum_copy(&base_copy, base);
  if (err < 0) {
    goto delete_from_base_copy;
  }
  bignum_t exp_copy;
  bignum_init(&exp_copy);
  err = bignum_copy(&exp_copy, exp);
  if (err < 0) {
    goto delete_from_exp_copy;
  }
  bignum_t zero;
  bignum_init(&zero);
  err = bignum_set_zero(&zero);
  if (err < 0) {
    goto delete_all;
  }
  while (bignum_cmp_abs(&exp_copy, &zero) == 1) {
    /* Если очередной разряд степени 1, тогда умножаем результат
     * на это умножение. Иначе копим дальше. */
    if (!bignum_is_even(&exp_copy)) {
      err = bignum_mul(&res_copy, &res_copy, &base_copy);
      if (err < 0) {
        goto delete_all;
      }
    }
    /* Возводим основание в квадрат*/
    err = bignum_mul(&base_copy, &base_copy, &base_copy);
    if (err < 0) {
      goto delete_all;
    }
    bignum_shift_right_bits(&exp_copy, 1);
  }
  err = bignum_copy(res, &res_copy);
  if (err < 0) {
    goto delete_all;
  }
delete_all:
  bignum_delete(&zero);
delete_from_exp_copy:
  bignum_delete(&exp_copy);
delete_from_base_copy:
  bignum_delete(&base_copy);
delete_from_res_copy:
  bignum_delete(&res_copy);
  return err;
}

int bignum_sqrt(bignum_t *res, const bignum_t *val) {
  int err = 0;
  if (!res || !val) {
    return -ERR_NULLPTR;
  }
  /* Тривиальные случаи */
  /* Отрицательный аргумент */
  if (val->negative) {
    return -ERR_INVALID_PARAM;
  }
  /* Нуль */
  if (bignum_is_zero(val)) {
    err = bignum_set_zero(res);
    return err;
  }
  /* Единица */
  bignum_t one;
  bignum_init(&one);
  err = bignum_from_ui8_array(&one, (uint8_t[]){1}, 1, 0);
  if (err < 0) {
    bignum_delete(&one);
    return err;
  }
  if (bignum_cmp(val, &one) == 0) {
    err = bignum_copy(res, val);
    bignum_delete(&one);
    return err;
  }
  /* Реализуется итеративная формула :
   * - y = (x + val / x) / 2
   * - x = y
   * Если y >= x, тогда x является ответом
   * Для начального приближения x берется значение
   * x = 2^((bits + 1) / 2) */
  bignum_t x, y;
  bignum_init(&x), bignum_init(&y);
  err = bignum_from_ui8_array(&x, (uint8_t[]){1}, 1, 0);
  if (err < 0) {
    goto cleanup;
  }
  size_t bits = (val->length - 1) * 32 +
                (32 - bignum_leading_zeros(val->val[val->length - 1]));
  err = bignum_shift_left_bits(&x, (bits + 1) / 2);
  if (err < 0) {
    goto cleanup;
  }
  while (1) {
    err = bignum_div(&y, val, &x);
    if (err < 0)
      goto cleanup;
    err = bignum_add(&y, &y, &x);
    if (err < 0)
      goto cleanup;
    bignum_shift_right_bits(&y, 1);

    int cmp = bignum_cmp_abs(&y, &x);
    if (cmp > -1) {
      break;
    }

    err = bignum_copy(&x, &y);
    if (err < 0)
      goto cleanup;
  }
  err = bignum_copy(res, &x);
cleanup:
  bignum_delete(&x);
  bignum_delete(&y);
  return err;
}

int bignum_gcd(bignum_t *res, const bignum_t *val_1, const bignum_t *val_2) {
  if (!res || !val_1 || !val_2) {
    return -ERR_NULLPTR;
  }
  int err = 0;
  bignum_t r, a, b;
  bignum_init(&r);
  bignum_init(&a);
  bignum_init(&b);
  err = bignum_copy(&a, val_1);
  if (err < 0) {
    goto delete_all;
  }
  err = bignum_copy(&b, val_2);
  if (err < 0) {
    goto delete_all;
  }
  /* Реализуется алгоритм Евклида:
   * r = a mod b
   * a = b
   * b = r
   * Если b = 0, то a содержит НОД
   */
  while (!bignum_is_zero(&b)) {
    err = bignum_mod(&r, &a, &b);
    if (err < 0) {
      goto delete_all;
    }
    err = bignum_copy(&a, &b);
    if (err < 0) {
      goto delete_all;
    }
    err = bignum_copy(&b, &r);
    if (err < 0) {
      goto delete_all;
    }
  }
  err = bignum_copy(res, &a);
  if (err < 0) {
    goto delete_all;
  }
  res->negative = false;
delete_all:
  bignum_delete(&b);
  bignum_delete(&a);
  bignum_delete(&r);
  return err;
}
/* lcm(a, b) = (|a| / gcd(a, b)) * |b|.
 * Используем деление до умножения, чтобы промежуточные числа были меньше. */
int bignum_lcm(bignum_t *res, const bignum_t *val_1, const bignum_t *val_2) {
  if (!res || !val_1 || !val_2) {
    return -ERR_NULLPTR;
  }
  int err = 0;
  bignum_t g;
  bignum_init(&g);
  err = bignum_gcd(&g, val_1, val_2);
  if (err < 0) {
    goto delete_all;
  }
  err = bignum_div(res, val_1, &g);
  if (err < 0) {
    goto delete_all;
  }
  err = bignum_mul(res, res, val_2);
  if (err < 0) {
    goto delete_all;
  }
  bignum_normalize(res);
  res->negative = false;
delete_all:
  bignum_delete(&g);
  return err;
}

int bignum_mod_ui32(uint32_t *res, const bignum_t *val, uint32_t divisor) {
  if (!res || !val)
    return -ERR_NULLPTR;
  if (divisor == 0)
    return -ERR_DIVISION_BY_ZERO;

  uint64_t r = 0;
  for (size_t i = val->length; i > 0; i--) {
    r = ((r << 32) | val->val[i - 1]) % divisor;
  }
  *res = (uint32_t)r;
  return 0;
}
