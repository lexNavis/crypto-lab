#include "bignum.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gmp.h>

/* Обеспечивает needed число разрядов в числе val.
 * Если разрядов не хватает, расширяет его */
static int bignum_ensure_capacity(bignum_t *val, size_t needed)
{
  if (val->capacity >= needed)
  {
    return 0;
  }
  /* Для новых чисел начальная выделяемая емкость - 1 ячейка */
  size_t new_cap = val->capacity ? val->capacity : 1;
  /* Удваиваем емкость, пока она не станет покрывать нужный размер */
  while (new_cap < needed)
  {
    new_cap *= 2;
  }
  uint32_t *new_val = realloc(val->val, new_cap * sizeof(uint32_t));
  if (!new_val)
  {
    return -ERR_REALLOC_FAILED;
  }
  val->val = new_val;
  val->capacity = new_cap;
  return 0;
}

/* Уменьшает разрядность числа val за счет игнорирования нулевых значащих разрядов */
static void bignum_normalize(bignum_t *val)
{
  while (val->length > 1 && val->val[val->length - 1] == 0)
  {
    val->length--;
  }
  if (bignum_is_zero(val))
  {
    val->negative = false;
  }
}

/* Возвращает численное значение hex символа, если 
 * таковое имеется */
static int hexval(const char chr)
{
  if (chr >= '0' && chr <= '9')
  {
    return chr - '0';
  }
  if (chr >= 'a' && chr <= 'f')
  {
    return chr - 'a' + 10;
  }
  if (chr >= 'A' && chr <= 'F')
  {
    return chr - 'A' + 10;
  }
  return -1;
}
/* Добавляет число val в качестве нового старшего разряда числа dest */
static int bignum_push(bignum_t *dest, uint32_t val)
{
  int err = bignum_ensure_capacity(dest, dest->length + 1);
  if (err < 0)
  {
    return err;
  }
  dest->val[dest->length] = val;
  dest->length++;
  return 0;
}
/* Беззнаковое сравнение двух чисел: 1 - первое число больше, 
 * -1 - второе число больше, 0 - равенство чисел */
static int bignum_cmp_abs(const bignum_t *val_1, const bignum_t *val_2)
{
  if (val_1->length > val_2->length)
    return 1;
  else if (val_1->length < val_2->length)
    return -1;
  else
  {
    for (size_t i = val_1->length; i > 0; i--)
    {
      if (val_1->val[i - 1] > val_2->val[i - 1])
        return 1;
      else if (val_1->val[i - 1] < val_2->val[i - 1])
        return -1;
    }
    return 0;
  }
}
/* Беззнаковое сложение чисел с записью результата в res */
static int bignum_add_abs(bignum_t *res, const bignum_t *val_1, const bignum_t *val_2)
{
  int err = 0;
  bignum_t res_copy;
  bignum_init(&res_copy);
  /* Сумма двух чисел не превышает разрядность наибольшего из них + 1
   * Доп разряд понадобится для помещения единицы переноса */
  size_t max_len;
  const bignum_t *max_val;
  if (val_1->length < val_2->length)
  {
    max_len = val_2->length;
    max_val = val_2; /* Сохраним для копирования без сложения */
  }
  else
  {
    max_len = val_1->length;
    max_val = val_1;
  }
  err = bignum_ensure_capacity(&res_copy, max_len + 1);
  if (err < 0)
  {
    bignum_delete(&res_copy);
    return err;
  }
  size_t min_len = (val_1->length < val_2->length) ? val_1->length : val_2->length;
  uint32_t carry = 0;
  /* Основной цикл сложения: здесь младшая часть максимального числа
   * складывается со вторым числом */
  for (size_t i = 0; i < min_len; i++)
  {
    uint64_t sum = (uint64_t)val_1->val[i] + val_2->val[i] + carry;
    res_copy.val[i] = (uint32_t)sum;
    carry = (uint32_t)(sum >> 32);
  }
  /* Цикл без сложения (но с учетом единицы переноса) */
  for (size_t i = min_len; i < max_len; i++)
  {
    uint64_t sum = (uint64_t)max_val->val[i] + carry;
    res_copy.val[i] = (uint32_t)sum;
    carry = (uint32_t)(sum >> 32);
  }
  res_copy.length = max_len;
  /* Если есть единица переноса из старшего разряда, добавим ее здесь */
  if (carry)
  {
    res_copy.val[max_len] = carry;
    res_copy.length++;
  }

  bignum_normalize(&res_copy);
  err = bignum_copy(res, &res_copy);
  if (err < 0)
  {
    bignum_delete(&res_copy);
    return err;
  }
  bignum_delete(&res_copy);
  return 0;
}
/* Беззнаковое вычитание чисел с записью результата в res. 
 * |a| >= |b| - должно гарантироваться для положительного результата
 * и корректного вычитания */
static int bignum_sub_abs(bignum_t *res, const bignum_t *val_1, const bignum_t *val_2)
{
  int err;
  bignum_t res_copy;
  bignum_init(&res_copy);
  err = bignum_ensure_capacity(&res_copy, val_1->length);
  if (err < 0)
  {
    bignum_delete(&res_copy);
    return err;
  }
  int borrow = 0;
  /* Основной цикл вычитания */
  for (size_t i = 0; i < val_2->length; i++)
  {
    int64_t diff = (int64_t)val_1->val[i] - val_2->val[i] - borrow;
    if (diff < 0)
    {
      /* Если результат суммы отрицательный, берем по модулю 2^32 */
      res_copy.val[i] = (uint32_t)(diff + (1LL << 32));
      borrow = 1;
    }
    else
    {
      res_copy.val[i] = (uint32_t)(diff);
      borrow = 0;
    }
  }
  /* Переписываем старшую часть с учетом единицы занятости */
  for (size_t i = val_2->length; i < val_1->length; i++)
  {
    int64_t diff = (int64_t)val_1->val[i] - borrow;

    if (diff < 0)
    {
      /* Если результат суммы отрицательный, берем по модулю 2^32 */
      res_copy.val[i] = (uint32_t)(diff + (1LL << 32));
      borrow = 1;
    }
    else
    {
      res_copy.val[i] = (uint32_t)(diff);
      borrow = 0;
    }
  }
  res_copy.negative = false;
  res_copy.length = val_1->length;
  bignum_normalize(&res_copy);
  err = bignum_copy(res, &res_copy);
  if (err < 0)
  {
    bignum_delete(&res_copy);
    return err;
  }
  bignum_delete(&res_copy);
  return 0;
}
/* Умножения числа bignum_t на единичный разряд с записью в res*/
static int bignum_mul_digit(bignum_t *res, const bignum_t *val, uint32_t digit)
{
  /* Результат умножение на "цифру" - возможное увеличение числа на разряд */
  bignum_t res_copy;
  bignum_init(&res_copy);
  int err = bignum_ensure_capacity(&res_copy, val->length + 1);
  if (err < 0)
  {
    bignum_delete(&res_copy);
    return err;
  }
  uint64_t carry = 0;
  for (size_t i = 0; i < val->length; i++)
  {
    uint64_t mul = (uint64_t)val->val[i] * digit + carry;
    res_copy.val[i] = (uint32_t)mul;
    carry = mul >> 32;
  }
  res_copy.length = val->length;
  /* Если в конце осталась единица переноса, добавим ее
   * и увеличим разрядность на 1 */
  if (carry)
  {
    res_copy.val[val->length] = carry;
    res_copy.length++;
  }
  res_copy.negative = false;
  bignum_normalize(&res_copy);
  err = bignum_copy(res, &res_copy);
  if (err < 0)
  {
    bignum_delete(&res_copy);
    return err;
  }
  bignum_delete(&res_copy);
  return 0;
}

/* Сдвиг влево на n разрядов */
static int bignum_shift_left(bignum_t *a, size_t n)
{
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
static void bignum_shift_right(bignum_t *a, size_t n)
{
  if (n == 0)
    return;
  if (n >= a->length)
  {
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
static int bignum_set_zero(bignum_t *val)
{
  int err = bignum_ensure_capacity(val, 1);
  if (err < 0)
    return err;
  val->val[0] = 0;
  val->length = 1;
  val->negative = false;
  return 0;
}

/* Сдвиг влево на n бит */
static int bignum_shift_left_bits(bignum_t *a, size_t n)
{
  size_t word_count = n / 32;
  size_t bit_count = n % 32;
  /* Сначала сдвинем биты. Величина битового сдвига
   * не больше слова, поэтому вектор может увеличиться
   * максимум на 1 разряд. */
  int err = bignum_ensure_capacity(a, a->length + word_count + 1);
  if (err < 0)
  {
    return err;
  }
  uint32_t carry = 0;
  if (bit_count > 0)
  {
    for (size_t i = 0; i < a->length; i++)
    {
      /* Сохраним исходное число */
      uint32_t cur = a->val[i];
      /* Заполним освободившиеся биты прошлым переносом после сдвига */
      a->val[i] = (a->val[i] << bit_count) | carry;
      /* Вычислим новый перенос, который будет сдвинут в левый разряд */
      carry = (cur >> (32 - bit_count));
    }
    if (carry != 0)
    {
      a->val[a->length] = carry;
      a->length++;
    }
  }
  /* Теперь слова */
  err = bignum_shift_left(a, word_count);
  return err;
}

/* Сдвиг вправо на n бит */
static void bignum_shift_right_bits(bignum_t *a, size_t n)
{
  size_t word_count = n / 32;
  size_t bit_count = n % 32;
  uint32_t carry = 0;
  if (bit_count > 0)
  {
    for (size_t i = a->length; i > 0; i--)
    {
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
static int bignum_leading_zeros(uint32_t x)
{
  if (x == 0)
    return 32;
  return __builtin_clz(x);
}

void bignum_init(bignum_t *val)
{
  val->capacity = 0;
  val->length = 0;
  val->negative = false;
  /* init не иницализирует число */
  val->val = NULL;
}

void bignum_delete(bignum_t *val)
{
  free(val->val);
  val->val = NULL;
  val->capacity = 0;
  val->length = 0;
  val->negative = false;
}

int bignum_copy(bignum_t *dest, const bignum_t *src)
{
  if (!dest || !src)
    return -ERR_NULLPTR;
  if (dest == src)
    return 0;
  int err = bignum_ensure_capacity(dest, src->length);
  if (err < 0)
  {
    return err;
  }
  for (size_t i = 0; i < src->length; i++)
  {
    dest->val[i] = src->val[i];
  }
  dest->negative = src->negative;
  dest->length = src->length;
  bignum_normalize(dest);
  return 0;
}

void bignum_print(const bignum_t *val)
{
  if (val->negative)
    printf("-");
  for (size_t i = val->length; i > 0; i--)
  {
    printf("%08x", val->val[i - 1]);
  }
  printf("\n");
}

void bignum_print_dec(const bignum_t *val) {
    if (bignum_is_zero(val)) {
        printf("0\n");
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
    printf("\n");

    free(parts);
    bignum_delete(&tmp);
    bignum_delete(&divisor);
    bignum_delete(&rem);
}

int bignum_from_ui8_array(bignum_t *dest, const uint8_t *src, size_t size, bool neg) {
  int err;
  if (!dest || !src)
  {
    return -ERR_NULLPTR;
  }
  if (size == 0)
  {
    return -ERR_INVALID_SIZE;
  }
  size_t word_count = (size + 3) / 4;
  err = bignum_ensure_capacity(dest, word_count);
  if (err != 0)
  {
    return err;
  }
  for (size_t i = 0; i < word_count; i++) {
     /* Так как word_count округлен до большего числа,
      * то максимальный элемент в src может быть на отрезке
      * [4*i, 4*i+3] в зависимости от округления. Для предотвращения
      * segfault вводятся проверки для каждого разряда */
      uint32_t b0 = (4*i     < size) ? src[4*i]     : 0;
      uint32_t b1 = (4*i + 1 < size) ? src[4*i + 1] : 0;
      uint32_t b2 = (4*i + 2 < size) ? src[4*i + 2] : 0;
      uint32_t b3 = (4*i + 3 < size) ? src[4*i + 3] : 0;
      dest->val[i] = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
  }
  dest->negative = neg;
  dest->length = word_count;
  bignum_normalize(dest);
  return 0; 
}

int bignum_from_ui32_array(bignum_t *dest, const uint32_t *src, size_t size, bool neg)
{
  int err;
  if (!dest || !src)
  {
    return -ERR_NULLPTR;
  }
  if (size == 0)
  {
    return -ERR_INVALID_SIZE;
  }
  err = bignum_ensure_capacity(dest, size);
  if (err != 0)
  {
    return err;
  }
  for (size_t i = 0; i < size; i++)
  {
    dest->val[i] = src[i];
  }
  dest->negative = neg;
  dest->length = size;
  bignum_normalize(dest);
  return 0;
}

int bignum_from_hex_string(bignum_t *dest, const char *hex, bool neg)
{
  if (!dest || !hex)
    return -ERR_NULLPTR;
  size_t n = strlen(hex);
  if (n == 0)
    return -ERR_INVALID_SIZE;

  dest->length = 0;
  uint32_t digit = 0;
  int shift = 0;

  for (size_t i = 0; i < n; i++)
  {
    int byte = hexval(hex[n - 1 - i]);
    if (byte < 0)
    {
      dest->length = 0;
      dest->negative = false;
      return -ERR_INVALID_STRING;
    }
    /* Символ занимает 4 бита, shift * 4 определяет 1 из 8 позиций в digit */
    digit |= (uint32_t)byte << (shift * 4);
    shift++;
    if (shift == 8)
    {
      int err = bignum_push(dest, digit);
      if (err < 0)
      {
        dest->length = 0;
        dest->negative = false;
        return err;
      }
      shift = 0;
      digit = 0;
    }
  }

  if (shift > 0)
  {
    int err = bignum_push(dest, digit);
    if (err < 0)
    {
      dest->length = 0;
      dest->negative = false;
      return err;
    }
  }

  dest->negative = neg;
  bignum_normalize(dest);
  return 0;
}

int bignum_from_gmp(bignum_t *dest, const mpz_t src)
{
  if (!dest)
  {
    return -ERR_NULLPTR;
  }
  int err = 0;
  int sign = mpz_sgn(src);
  if (sign == 0)
  {
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

long bignum_to_ui8_array(uint8_t *dest, size_t dest_size, const bignum_t *src)
{
  if (!dest || !src)
  {
    return -ERR_NULLPTR;
  }
  if (dest_size < src->length * 4)
  {
    return -ERR_INVALID_SIZE;
  }
  for (size_t i = 0; i < src->length; i++) {
    dest[4 * i] = src->val[i] & 0xFF;
    dest[4 * i + 1] = (src->val[i] >> 8) & 0xFF;
    dest[4 * i + 2] = (src->val[i] >> 16) & 0xFF;
    dest[4 * i + 3] = (src->val[i] >> 24) & 0xFF;
  }
  return (long)src->length * 4;
}

long bignum_to_ui32_array(uint32_t *dest, size_t dest_size, const bignum_t *src)
{
  if (!dest || !src)
  {
    return -ERR_NULLPTR;
  }
  if (dest_size < src->length)
  {
    return -ERR_INVALID_SIZE;
  }
  memcpy(dest, src->val, src->length * sizeof(uint32_t));
  return (long)src->length;
}

long bignum_to_hex_string(char *dest, size_t dest_size, const bignum_t *src)
{
  if (!dest || !src)
  {
    return -ERR_NULLPTR;
  }
  /* Проверка, влезет ли в строку указанное число ячеек
   * по 4 байта (8 символов) + терминальный ноль */
  if (dest_size < src->length * 8 + 1)
  {
    return -ERR_INVALID_SIZE;
  }
  for (size_t i = src->length; i > 0; i--)
  {
    snprintf(dest + (src->length - i) * 8, 9, "%08x", src->val[i - 1]);
  }
  dest[src->length * 8] = '\0';
  return (long)strlen(dest);
}

void bignum_to_gmp(mpz_t dest, const bignum_t *src)
{
  mpz_import(dest, src->length, -1, sizeof(uint32_t), 0, 0, src->val);
  if (src->negative)
  {
    mpz_neg(dest, dest);
  }
}

size_t bignum_length(const bignum_t *val)
{
  return val->length;
}
size_t bignum_capacity(const bignum_t *val)
{
  return val->capacity;
}

bool bignum_is_zero(const bignum_t *val)
{
  return (val->length == 1 && val->val[0] == 0);
}
bool bignum_is_even(const bignum_t *val)
{
  return (val->length > 0 && (val->val[0] & 1) == 0);
}

int bignum_cmp(const bignum_t *val_1, const bignum_t *val_2)
{
  if (val_1->negative != val_2->negative)
  {
    return val_1->negative ? -1 : 1;
  }

  int cmp = bignum_cmp_abs(val_1, val_2);

  /* Если оба отрицательные — инвертируем */
  if (val_1->negative)
  {
    cmp = -cmp;
  }
  return cmp;
}

int bignum_add(bignum_t *res, const bignum_t *val_1, const bignum_t *val_2)
{
  if (!res || !val_1 || !val_2)
  {
    return -ERR_NULLPTR;
  }
  int err;
  /* Сравнение знаков операндов */
  if (val_1->negative == val_2->negative)
  {
    /* Для одного знака производится сложение по модулю,
     * а знак берется у любого операнда */
    err = bignum_add_abs(res, val_1, val_2);
    if (err < 0)
    {
      return err;
    }
    res->negative = val_1->negative;
  }
  else
  {
    /* Для разных знаков производится вычитание меньшего модуля
     * из большего, а знак берется у большего по модулю числа */
    int cmp = bignum_cmp_abs(val_1, val_2);
    if (cmp >= 0)
    {
      err = bignum_sub_abs(res, val_1, val_2);
      if (err < 0)
      {
        return err;
      }
      res->negative = val_1->negative;
    }
    else
    {
      err = bignum_sub_abs(res, val_2, val_1);
      if (err < 0)
      {
        return err;
      }
      res->negative = val_2->negative;
    }
  }
  bignum_normalize(res);
  return 0;
}

int bignum_sub(bignum_t *res, const bignum_t *val_1, const bignum_t *val_2)
{
  if (!res || !val_1 || !val_2)
  {
    return -ERR_NULLPTR;
  }
  int err;
  /* Сравнение знаков операндов */
  if (val_1->negative != val_2->negative)
  {
    /* Для разных знаков производится сложение модулей,
     * а знак берется у первого числа */
    err = bignum_add_abs(res, val_1, val_2);
    if (err < 0)
    {
      return err;
    }
    res->negative = val_1->negative;
  }
  else
  {
    /* Для одного знака производится вычитание модулей,
     * а знак берется у наибольшего по модулю числа по правилу:
     * знак первого числа или инвертированный знак второго числа */
    int cmp = bignum_cmp_abs(val_1, val_2);
    if (cmp >= 0)
    {
      err = bignum_sub_abs(res, val_1, val_2);
      if (err < 0)
      {
        return err;
      }
      res->negative = val_1->negative;
    }
    else
    {
      err = bignum_sub_abs(res, val_2, val_1);
      if (err < 0)
      {
        return err;
      }
      res->negative = !val_2->negative;
    }
  }
  bignum_normalize(res);
  return 0;
}

int bignum_mul(bignum_t *res, const bignum_t *val_1, const bignum_t *val_2)
{
  if (!res || !val_1 || !val_2)
  {
    return -ERR_NULLPTR;
  }
  bignum_t res_copy;
  bignum_init(&res_copy);
  /* Результат промежуточных умножений */
  bignum_t mul;
  bignum_init(&mul);
  int err = bignum_ensure_capacity(&res_copy, val_1->length + val_2->length);
  if (err < 0)
  {
    goto delete_all;
  }
  /* Лучше в качестве множителя выбрать число с наименьшим числом разрядов */
  if (bignum_cmp_abs(val_1, val_2) == -1)
  {
    /* Если второе число больше, поменяем их локально местами */
    const bignum_t *tmp = val_1;
    val_1 = val_2;
    val_2 = tmp;
  }

  /* После сравнения гарантируется, что второе число - минимальное.
   * Обычное умножение подразумевает умножение первого числа на очередной
   * разряд второго, сдвига результата на число, равное индексу этого разряда
   * и сложение с результатом */
  for (size_t i = 0; i < val_2->length; i++)
  {
    err = bignum_mul_digit(&mul, val_1, val_2->val[i]);
    if (err < 0)
    {
      goto delete_all;
    }
    err = bignum_shift_left(&mul, i);
    if (err < 0)
    {
      goto delete_all;
    }
    err = bignum_add(&res_copy, &res_copy, &mul);
    if (err < 0)
    {
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

int bignum_divmod(bignum_t *quot, bignum_t *rem, const bignum_t *val_1, const bignum_t *val_2)
{
  if (!quot || !rem || !val_1 || !val_2)
  {
    return -ERR_NULLPTR;
  }
  /* Деление на ноль */
  if (bignum_is_zero(val_2))
  {
    return -ERR_DIVISION_BY_ZERO;
  }
  int err;
  /* Тривиальные случаи */
  int cmp = bignum_cmp_abs(val_1, val_2);
  /* val_1 < val_2 по модулю */
  if (cmp == -1)
  {
    err = bignum_set_zero(quot);
    if (err < 0)
    {
      return err;
    }
    err = bignum_copy(rem, val_1);
    if (err < 0)
    {
      return err;
    }
    return 0;
  }
  /* val_1 = val_2 по модулю */
  if (cmp == 0)
  {
    uint32_t one = 1;
    err = bignum_from_ui32_array(quot, &one, 1, val_1->negative ^ val_2->negative);
    if (err < 0)
    {
      return err;
    }
    err = bignum_set_zero(rem);
    if (err < 0)
    {
      return err;
    }
    return 0;
  }
  /* Основной цикл деления, реализованный на основе алгоритма Кнута */

  /* Определим величину сдвига, чтобы старший бит старшей части делителя стал 1 */
  int shift = bignum_leading_zeros(val_2->val[val_2->length - 1]);
  /* Локальные копии для сохранности входных параметров */
  bignum_t a;
  bignum_t b;
  bignum_init(&a);
  bignum_init(&b);
  err = bignum_copy(&a, val_1);
  if (err < 0)
  {
    return err;
  }
  err = bignum_copy(&b, val_2);
  if (err < 0)
  {
    return err;
  }

  err = bignum_shift_left_bits(&b, shift);
  if (err < 0)
  {
    return err;
  }
  /* Для алгоритма критично появление у делимого нового разряда,
   * поэтому добавим его вручную */
  err = bignum_push(&a, 0);
  if (err < 0)
  {
    return err;
  }
  err = bignum_shift_left_bits(&a, shift);
  if (err < 0)
  {
    return err;
  }

  size_t dr_len = b.length; // divisor
  /* Окно алгоритма — n+1 слов, где n = dr_len (число разрядов делителя). 
   * Поэтому последняя позиция окна: m = length(a) - n - 1. */
  size_t m = a.length - b.length - 1;
  err = bignum_ensure_capacity(quot, m + 1);
  if (err < 0)
  {
    bignum_delete(&a);
    bignum_delete(&b);
    return err;
  }
  quot->length = m + 1;
  /* В рамках каждой итерации цикла рассматривается
   * фрагмент делимого [j..(j + dr_len)]*/
  for (size_t i = m + 1; i > 0; i--)
  {
    size_t j = i - 1;
    /* Поделим два старших разряда делимого на старший разряд
     * нормализованного делителя аппаратными средствами */
    uint64_t a_high = ((uint64_t)a.val[j + dr_len] << 32) | (uint64_t)a.val[j + dr_len - 1];
    uint64_t b_high = b.val[dr_len - 1];
    uint64_t result;
    uint64_t rest;
    bool is_overflow = false;
    /* Проверка, вызовет ли результат переполнение разрядной сетки*/
    if (a.val[j + dr_len] >= b.val[dr_len - 1])
    {
      /* В таком случае устанавливаем максимальное
       * число для частного */
      result = 0xFFFFFFFF;
      rest = 0;
      is_overflow = true;
    }
    else
    {
      result = a_high / b_high;
      rest = a_high % b_high;
    }
    /* Мы нашли частное для старшей части фрагмента.
     * Осталось обеспечить, чтобы частное подходило
     * для младшей части. (Только если нет переполнения)*/
    if (dr_len > 1 && !is_overflow)
    {
      /* Нужно обеспечить, чтобы произведение частного
       * на младшую часть делителя было не больше чем остаток
       * фрагмента делимого. Иначе разность будет отрицательная.
       * (При вычитании младшая часть займет 1 у старшей и последняя станет меньше
       * старшей части произведения) */
      while (result * b.val[dr_len - 2] > (rest << 32) + (uint64_t)a.val[j + dr_len - 2])
      {
        /* Уменьшаем частное и возвращаем остатку множители */
        result--;
        rest += b_high;
        /* Если остаток превысил 32 бита, можно досрочно выйти из цикла */
        if (rest >= (1ULL << 32))
        {
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
     * Уменьшаем result на 1 и возвращаем одно b обратно в остаток числа окна. */
    if (bignum_cmp_abs(&window, &prod) == -1)
    {
      result--;
      bignum_add(&window, &window, &b);
    }

    /* Теперь вычитаем. window = window - prod. */
    bignum_sub_abs(&window, &window, &prod);
    /* Записываем window обратно в u[j..j+dr_len]. */
    for (size_t k = 0; k < dr_len + 1; k++)
    {
      if (k < window.length)
      {
        a.val[j + k] = window.val[k];
      }
      else
      {
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
  if (err < 0)
  {
    bignum_delete(&a);
    bignum_delete(&b);
    return err;
  }
  /* Денормализация: сдвиг вправо на shift. */
  bignum_shift_right_bits(rem, shift);
  rem->negative = val_1->negative;
  bignum_normalize(rem);

  bignum_delete(&a);
  bignum_delete(&b);
  return 0;
}

/* Использует divmod, но остаток игнорирует */
int bignum_div(bignum_t *quot, const bignum_t *val_1, const bignum_t *val_2)
{
  bignum_t rem;
  bignum_init(&rem);
  int err = bignum_divmod(quot, &rem, val_1, val_2);
  bignum_delete(&rem);
  return err;
}
/* Использует divmod, но целую часть игнорирует */
int bignum_mod(bignum_t *rem, const bignum_t *val_1, const bignum_t *val_2)
{
  bignum_t quot;
  bignum_init(&quot);
  int err = bignum_divmod(&quot, rem, val_1, val_2);
  bignum_delete(&quot);
  return err;
}

int bignum_pow_mod(bignum_t *res, const bignum_t *base, const bignum_t *exp, const bignum_t *mod)
{
  if (!res || !base || !exp || !mod)
  {
    return -ERR_NULLPTR;
  }
  int err = 0;
  bignum_t res_copy;
  bignum_init(&res_copy);
  /* Установим начальное значение 1 для результата */
  uint32_t one = 1;
  err = bignum_from_ui32_array(&res_copy, &one, 1, false);
  if (err < 0)
  {
    goto delete_from_res_copy;
  }
  /* Копии изменяемых величин для сохранения исходников */
  bignum_t base_copy;
  bignum_init(&base_copy);
  err = bignum_copy(&base_copy, base);
  if (err < 0)
  {
    goto delete_from_base_copy;
  }
  bignum_t exp_copy;
  bignum_init(&exp_copy);
  err = bignum_copy(&exp_copy, exp);
  if (err < 0)
  {
    goto delete_from_exp_copy;
  }
  bignum_t zero;
  bignum_init(&zero);
  err = bignum_set_zero(&zero);
  if (err < 0)
  {
    goto delete_all;
  }
  err = bignum_mod(&base_copy, &base_copy, mod);
  if (err < 0)
  {
    goto delete_all;
  }
  /* Цикл по разрядам степени. Продолжаем, пока остались 
   * необраюотанные разряды */
  while (bignum_cmp_abs(&exp_copy, &zero) == 1)
  {
    /* Если очередной разряд степени 1, тогда умножаем результат
     * на это умножение. Иначе копим дальше. */
    if (!bignum_is_even(&exp_copy))
    {
      err = bignum_mul(&res_copy, &res_copy, &base_copy);
      if (err < 0)
      {
        goto delete_all;
      }
      err = bignum_mod(&res_copy, &res_copy, mod);
      if (err < 0)
      {
        goto delete_all;
      }
    }
    /* Возводим основание в квадрат по модулю mod */
    err = bignum_mul(&base_copy, &base_copy, &base_copy);
    if (err < 0)
    {
      goto delete_all;
    }
    err = bignum_mod(&base_copy, &base_copy, mod);
    if (err < 0)
    {
      goto delete_all;
    }
    bignum_shift_right_bits(&exp_copy, 1);
  }
  err = bignum_copy(res, &res_copy);
  if (err < 0)
  {
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

int bignum_pow(bignum_t *res, const bignum_t *base, const bignum_t *exp)
{
  if (!res || !base || !exp)
  {
    return -ERR_NULLPTR;
  }
  int err = 0;
  bignum_t res_copy;
  bignum_init(&res_copy);
  /* Установим начальное значение 1 для результата */
  uint32_t one = 1;
  err = bignum_from_ui32_array(&res_copy, &one, 1, false);
  if (err < 0)
  {
    goto delete_from_res_copy;
  }
  /* Копии изменяемых величин для сохранения исходников */
  bignum_t base_copy;
  bignum_init(&base_copy);
  err = bignum_copy(&base_copy, base);
  if (err < 0)
  {
    goto delete_from_base_copy;
  }
  bignum_t exp_copy;
  bignum_init(&exp_copy);
  err = bignum_copy(&exp_copy, exp);
  if (err < 0)
  {
    goto delete_from_exp_copy;
  }
  bignum_t zero;
  bignum_init(&zero);
  err = bignum_set_zero(&zero);
  if (err < 0)
  {
    goto delete_all;
  }
  while (bignum_cmp_abs(&exp_copy, &zero) == 1)
  {
    /* Если очередной разряд степени 1, тогда умножаем результат
     * на это умножение. Иначе копим дальше. */
    if (!bignum_is_even(&exp_copy))
    {
      err = bignum_mul(&res_copy, &res_copy, &base_copy);
      if (err < 0)
      {
        goto delete_all;
      }
    }
    /* Возводим основание в квадрат*/
    err = bignum_mul(&base_copy, &base_copy, &base_copy);
    if (err < 0)
    {
      goto delete_all;
    }
    bignum_shift_right_bits(&exp_copy, 1);
  }
  err = bignum_copy(res, &res_copy);
  if (err < 0)
  {
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

int bignum_gcd(bignum_t *res, const bignum_t *val_1, const bignum_t *val_2)
{
  if (!res || !val_1 || !val_2)
  {
    return -ERR_NULLPTR;
  }
  int err = 0;
  bignum_t r, a, b;
  bignum_init(&r);
  bignum_init(&a);
  bignum_init(&b);
  err = bignum_copy(&a, val_1);
  if (err < 0)
  {
    goto delete_all;
  }
  err = bignum_copy(&b, val_2);
  if (err < 0)
  {
    goto delete_all;
  }
  /* Реализуется алгоритм Евклида: 
   * r = a mod b
   * a = b
   * b = r
   * Если b = 0, то a содержит НОД 
   */
  while (!bignum_is_zero(&b))
  {
    err = bignum_mod(&r, &a, &b);
    if (err < 0)
    {
      goto delete_all;
    }
    err = bignum_copy(&a, &b);
    if (err < 0)
    {
      goto delete_all;
    }
    err = bignum_copy(&b, &r);
    if (err < 0)
    {
      goto delete_all;
    }
  }
  err = bignum_copy(res, &a);
  if (err < 0)
  {
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
int bignum_lcm(bignum_t *res, const bignum_t *val_1, const bignum_t *val_2)
{
  if (!res || !val_1 || !val_2)
  {
    return -ERR_NULLPTR;
  }
  int err = 0;
  bignum_t g;
  bignum_init(&g);
  err = bignum_gcd(&g, val_1, val_2);
  if (err < 0)
  {
    goto delete_all;
  }
  err = bignum_div(res, val_1, &g);
  if (err < 0)
  {
    goto delete_all;
  }
  err = bignum_mul(res, res, val_2);
  if (err < 0)
  {
    goto delete_all;
  }
  bignum_normalize(res);
  res->negative = false;
delete_all:
  bignum_delete(&g);
  return err;
}
