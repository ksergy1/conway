#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define ESC 1 /* XXX */

typedef uint64_t value_t;
typedef struct {
  value_t value;
  size_t bitCount;
} Bits_t;
typedef struct {
  Bits_t bits;
  value_t mask;
  size_t bitCountMax;
} Bit_generator_t;

/**** functions declarations ****/
// подбор кортежа B исходя из A формата width
value_t brute_force(value_t A, int width);
// шансы B перед A, ширина кортежей = width
double conway(value_t A, value_t B, int width);
// число (не пойми что) AB, ширина = width
value_t conway_single(value_t A, value_t B, int width);

// выдать случайный бит
value_t random_bit(void);
//
value_t random_value(int width);

//
void bg_init(Bit_generator_t *bg, size_t bc);
//
const Bits_t *bg_next_bit(Bit_generator_t *bg);

// ожидание кнопки и ее выдача
int keypress(void);
// вывод
void output(value_t A, value_t B, const Bits_t *bits, int width);

/**** functions definitions ****/
value_t conway_single(value_t A, value_t B, int width)
{
  int k;
  value_t maskA = (1 << width) - 1;
  value_t A_, B_;
  value_t ret;

  for (k = 0, A_ = A, B_ = B;
       k < width;
       maskA >>= 0x01, A_ = (A & maskA), B_ >>= 1, ++k) {
    ret = (ret | (A_ == B_)) << 0x01;
  }

  return ret;
}

double conway(value_t A, value_t B, int width)
{
  value_t AA, AB, BB, BA;
  AA = conway_single(A, A, width);
  AB = conway_single(A, B, width);
  BB = conway_single(B, B, width);
  BA = conway_single(B, A, width);

  return ((double)(AA - AB)) / ((double)(BB - BA));
}

value_t brute_force(value_t A, int width)
{
  value_t limit = 1 << width;
  value_t B;

  for (B = 0;
       (!(conway(A, B) > 0.5)) && (B < limit);
       ++B);

  return B;
}

void bg_init(Bit_generator_t *bg, size_t bc)
{
  memset(bg, 0, sizeof(Bit_generator_t));
  bg->bitCountMax = bc;
  bg->mask = (0x01 << bc) - 0x01;
}

const Bits_t *bg_next_bit(Bit_generator_t *bg)
{
  Bits_t *bits = &bg->bits;
  value_t ls_bit = random_bit();
  bits->value = ((bits->value << 0x01) | ls_bit) & bg->mask;
  if (bits->bitCount < bg->bitCountMax) ++bits->bitCount;

  return bits;
}

int main(int argc, char **argv)
{
  /* usage: argv[0] <width> */
  value_t A;
  value_t B;
  int kp;
  int width;
  Bit_generator_t bg;
  const Bits_t *bits;

  do {
    A = random_value(width);
    B = brute_force(A, width);

    bg_init(&bg, width);

    do {
      bits = bg_next_bit(bg);
      output(A, B, bits, width);
      kp = keypress();
    } while ((kp != ESC) &&
             ((bits->bitCount < width) ||
              ((A != bits->value) &&
               (B != bits->value))));

    kp = keypress();
  } (kp != ESC);

  return 0;
}
