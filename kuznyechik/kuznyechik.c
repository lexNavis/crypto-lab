#include <string.h>
#include <stdio.h>
#include "kuznyechik.h"
#include "../common/entropy.h"

static const uint8_t pi[256] = {
    252, 238, 221, 17, 207, 110, 49, 22, 251, 196, 250, 218, 35, 197, 4, 77, 233, 119, 240, 219, 147,
    46, 153, 186, 23, 54, 241, 187, 20, 205, 95, 193, 249, 24, 101, 90, 226, 92, 239, 33, 129, 28, 60, 66,
    139, 1, 142, 79, 5, 132, 2, 174, 227, 106, 143, 160, 6, 11, 237, 152, 127, 212, 211, 31, 235, 52, 44, 81,
    234, 200, 72, 171, 242, 42, 104, 162, 253, 58, 206, 204, 181, 112, 14, 86, 8, 12, 118, 18, 191, 114, 19,
    71, 156, 183, 93, 135, 21, 161, 150, 41, 16, 123, 154, 199, 243, 145, 120, 111, 157, 158, 178, 177, 50,
    117, 25, 61, 255, 53, 138, 126, 109, 84, 198, 128, 195, 189, 13, 87, 223, 245, 36, 169, 62, 168, 67,
    201, 215, 121, 214, 246, 124, 34, 185, 3, 224, 15, 236, 222, 122, 148, 176, 188, 220, 232, 40, 80, 78,
    51, 10, 74, 167, 151, 96, 115, 30, 0, 98, 68, 26, 184, 56, 130, 100, 159, 38, 65, 173, 69, 70, 146, 39,
    94, 85, 47, 140, 163, 165, 125, 105, 213, 149, 59, 7, 88, 179, 64, 134, 172, 29, 247, 48, 55, 107, 228,
    136, 217, 231, 137, 225, 27, 131, 73, 76, 63, 248, 254, 141, 83, 170, 144, 202, 216, 133, 97, 32, 113,
    103, 164, 45, 43, 9, 91, 203, 155, 37, 208, 190, 229, 108, 82, 89, 166, 116, 210, 230, 244, 180, 192,
    209, 102, 175, 194, 57, 75, 99, 182};

/* pi_inv[pi[x]] = x */
static const uint8_t pi_inv[256] = {
    165, 45, 50, 143, 14, 48, 56, 192, 84, 230, 158, 57, 85, 126, 82, 145, 100, 3, 87, 90, 28, 96, 7, 24, 33,
    114, 168, 209, 41, 198, 164, 63, 224, 39, 141, 12, 130, 234, 174, 180, 154, 99, 73, 229, 66, 228, 21, 183,
    200, 6, 112, 157, 65, 117, 25, 201, 170, 252, 77, 191, 42, 115, 132, 213, 195, 175, 43, 134, 167, 177, 178,
    91, 70, 211, 159, 253, 212, 15, 156, 47, 155, 67, 239, 217, 121, 182, 83, 127, 193, 240, 35, 231, 37, 94, 181,
    30, 162, 223, 166, 254, 172, 34, 249, 226, 74, 188, 53, 202, 238, 120, 5, 107, 81, 225, 89, 163, 242, 113, 86,
    17, 106, 137, 148, 101, 140, 187, 119, 60, 123, 40, 171, 210, 49, 222, 196, 95, 204, 207, 118, 44, 184, 216, 46,
    54, 219, 105, 179, 20, 149, 190, 98, 161, 59, 22, 102, 233, 92, 108, 109, 173, 55, 97, 75, 185, 227, 186, 241, 160,
    133, 131, 218, 71, 197, 176, 51, 250, 150, 111, 110, 194, 246, 80, 255, 93, 169, 142, 23, 27, 151, 125, 236, 88, 247,
    31, 251, 124, 9, 13, 122, 103, 69, 135, 220, 232, 79, 29, 78, 4, 235, 248, 243, 62, 61, 189, 138, 136, 221, 205, 11, 19,
    152, 2, 147, 128, 144, 208, 36, 52, 203, 237, 244, 206, 153, 16, 68, 64, 146, 58, 1, 38, 18, 26, 72, 104, 245, 129,
    139, 199, 214, 32, 10, 8, 0, 76, 215, 116};

/* Умножение в GF(2^8) по модулю p(x) = x^8 + x^7 + x^6 + x + 1. */
static void gf_2x_px_mul(uint16_t *res, uint8_t arg_1, uint8_t arg_2)
{
  /* Для перемножения двух чисел второе из них (arg_2)
   * представляется в виде многочлена с коэффициентами
   * из поля GF(2): x^n + {1,0}*x^(n-1) + ... + {1,0}*x + {1,0}
   * За сложение отвечает операция XOR, а за расчёт x^i - сдвиг:
   * x^i = arg_2 << i */
  for (int i = 0; i < 8; i++)
  {
    /* Члены с нулевыми коэффициентами пропускаются */
    if (((arg_2 >> i) & 1) == 0)
    {
      continue;
    }
    /* Накопление суммы через XOR */
    (*res) ^= (arg_1 << i);
  }
  /* p тоже представлена в виде многочлена
   * 1 * x^8 + 1 * x^7 + 1 * x^6 + 1 * x + 1
   * Но, как и в случае с аргументами, достаточно
   * хранить коэффициенты.
   */
  uint16_t p = 0b111000011;
  /* Полученный ранее результат (многочлен) может иметь степень выше 7.
   * В таком виде он не сможет быть представлен в виде 8 битного числа,
   * поэтому мы используем p, чтобы понизить степень результата до 7 или ниже,
   * осуществив деление */
  do
  {
    int i;
    /* Для этого найдем первый ненулевой бит результата слева.
     * Он определит степень результата. */
    for (i = 15; i > 7; i--)
    {
      if ((((*res) >> i) & 1) != 0)
      {
        break;
      }
    }
    /* Если степень уже не выше 7, то выходим */
    if (i == 7)
    {
      // printf("Res: %04x\n", *res);
      break;
    }
    /* В ином случае мы осуществляем деление методом столбика:
     * Сдвигаем p, чтобы совместить старшие биты результата и p, потом
     * используем операцию XOR для вычитания */
    (*res) ^= (p << (i - 8));
  } while (1);
}

/* out = X[module](arg) = m_i ^ a_i (i = 0, 15)*/
static void op_x(uint8_t out[16], const uint8_t arg[16], const uint8_t module[16])
{
  for (int i = 0; i < 16; i++)
  {
    out[i] = arg[i] ^ module[i];
  }
}
/* out = S(arg) = pi_i(arg) (i = 0, 15)*/
void op_s(uint8_t out[16], const uint8_t arg[16])
{
  for (int i = 0; i < 16; i++)
  {
    out[i] = pi[arg[i]];
  }
}
/* out = S_inv(arg) = pi_inv_i(arg) (i = 0, 15)*/
static void op_s_inv(uint8_t out[16], const uint8_t arg[16])
{
  for (int i = 0; i < 16; i++)
  {
    out[i] = pi_inv[arg[i]];
  }
}
/* l(a_15,..,a_0) = sum((ki * a15-i) mod p(x))*/
static void op_linear(uint8_t *out, const uint8_t arg[16])
{
  *out = 0;
  uint8_t koefs[16] = {
      148, 32, 133, 16,
      194, 192, 1, 251,
      1, 192, 194, 16,
      133, 32, 148, 1};
  for (int i = 0; i < 16; i++)
  {
    uint16_t res_mul = 0;
    gf_2x_px_mul(&res_mul, koefs[i], arg[15 - i]);
    (*out) ^= (uint8_t)res_mul;
  }
}
/* Сдвиг вправо аргумента и запись L(a) в старший байт */
static void op_r(uint8_t out[16], const uint8_t arg[16])
{

  uint8_t tmp;
  op_linear(&tmp, arg);
  for (int i = 0; i < 15; i++)
  {
    out[i] = arg[i + 1];
  }
  out[15] = tmp;
}
/* Выполнение операции R 16 раз над самим собой */
static void op_l(uint8_t out[16], const uint8_t arg[16])
{
  /* Первую итерацию надо провести с внешним
   * аргументом, а остальные 15 проводятся сами
   * над собой в цикле */
  op_r(out, arg);
  for (int i = 1; i < 16; i++)
  {
    op_r(out, out);
  }
}
/* Обратная R операция */
static void op_r_inv(uint8_t out[16], const uint8_t arg[16])
{
  /* Сделать копию для изменения порядка элементов */
  uint8_t arg_mod[16];
  arg_mod[0] = arg[15];
  for (int i = 15; i > 0; i--)
  {
    arg_mod[i] = arg[i - 1];
  }
  uint8_t tmp;
  op_linear(&tmp, arg_mod);
  arg_mod[0] = tmp;
  memcpy(out, arg_mod, 16);
}

/* Обратная L операция */
static void op_l_inv(uint8_t out[16], const uint8_t arg[16])
{
  op_r_inv(out, arg);
  for (int i = 1; i < 16; i++)
  {
    uint8_t out_cpy[16];
    op_r_inv(out_cpy, out);
    memcpy(out, out_cpy, 16);
  }
}
/* Композция функций LSX[mod](a) = L(S(X[mod](a))) */
static void op_LSX(uint8_t arg_1[16], const uint8_t module[16])
{
  op_x(arg_1, arg_1, module);
  op_s(arg_1, arg_1);
  op_l(arg_1, arg_1);
}
/* Композция функций S^(-1)L^(-1)X[mod](a). Обратна LSX */
static void op_LSX_inv(uint8_t arg_1[16], const uint8_t module[16])
{
  op_x(arg_1, arg_1, module);
  op_l_inv(arg_1, arg_1);
  op_s_inv(arg_1, arg_1);
}

/* F[mod](a2, a1) = {LSX(a2)^a1, a2} */
static void op_f(uint8_t arg_1[16], uint8_t arg_2[16], const uint8_t module[16])
{
  uint8_t new_arg_1[16];

  memcpy(new_arg_1, arg_1, 16);
  op_LSX(new_arg_1, module);
  for (int i = 0; i < 16; i++)
  {
    new_arg_1[i] ^= arg_2[i];
  }
  memcpy(arg_2, arg_1, 16);
  memcpy(arg_1, new_arg_1, 16);
}

/* Создание 16 байтового числа из 1 байтового */
static void itob(uint8_t out[16], uint8_t val)
{
  memset(out, 0, 16);
  out[0] = val;
}

/* Основной процесс генерации ключей */
static void do_keygen(kuz_key_t *key, const uint8_t raw[32])
{
  memcpy(key->k, raw, 32);
  memcpy((key->k_arr)[0], (key->k) + 16, 16);
  memcpy((key->k_arr)[1], key->k, 16);
  for (int i = 0; i < 4; i++)
  {
    uint8_t k_1_new[16];
    uint8_t k_2_new[16];
    memcpy(k_1_new, (key->k_arr)[2 * i], 16);
    memcpy(k_2_new, (key->k_arr)[2 * i + 1], 16);
    for (int j = 0; j < 8; j++)
    {
      uint8_t c = 8 * i + j + 1;
      uint8_t c_blk[16];
      itob(c_blk, c);
      op_l(c_blk, c_blk);
      op_f(k_1_new, k_2_new, c_blk);
    }
    memcpy((key->k_arr)[2 * i + 2], k_1_new, 16);
    memcpy((key->k_arr)[2 * i + 3], k_2_new, 16);
  }
}

int kuz_keygen(kuz_key_t *key)
{
  unsigned char raw[32];
  int err = read_random_bytes(raw, 32);
  if (err < 0)
  {
    /* Handle error */
    return err;
  }
  do_keygen(key, raw);
  return 0;
}

int kuz_keygen_from_raw(kuz_key_t *key, const uint8_t raw[32])
{
  do_keygen(key, raw);
  return 0;
}

int kuz_encrypt(uint8_t block[16], const kuz_key_t *key)
{
  for (int i = 0; i < 9; i++)
  {
    op_LSX(block, key->k_arr[i]);
  }
  op_x(block, block, key->k_arr[9]);
  return 0;
}

int kuz_decrypt(uint8_t block[16], const kuz_key_t *key)
{
  for (int i = 9; i > 0; i--)
  {
    op_LSX_inv(block, key->k_arr[i]);
  }
  op_x(block, block, key->k_arr[0]);
  return 0;
}