#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint64_t value_t;

/* functions declarations */
// подбор кортежа B исходя из A формата width
value_t bruteForce(value_t A, int width);
// шансы B перед A, ширина кортежей = width
double conway(value_t A, value_t B, int width);
// число (не поями что) AB, ширина = width
value_t conwaySingle(value_t A, value_t B, int width);

/* functions definitions */
value_t conwaySingle(value_t A, value_t B, int width)
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
  AA = conwaySingle(A, A, width);
  AB = conwaySingle(A, B, width);
  BB = conwaySingle(B, B, width);
  BA = conwaySingle(B, A, width);

  return ((double)(AA - AB)) / ((double)(BB - BA));
}

value_t bruteForce(value_t A, int width)
{
  value_t limit = 1 << width;
  value_t B;

  for (B = 0;
       (!(conway(A, B) > 0.5)) && (B < limit);
       ++B);

  return B;
}

int main(int argc, char **argv)
{
  /*********************************
   * шаблон:
  do {
    A = random;
    B = bruteForce(A, width);
    test = 0;
    nextBit(0, 0); // reset nextBit internals
    do {
      nextBit(&test, width);
      output(A, B, test, width);
    } (key != quit && (A != test && B != test));
  } (key == continue);
   *
   *********************************/
  return 0;
}
