#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>

#define ESC                   '\033'
#define END_OF_FILE           '\004'
#define CURSOR_TO_UL          "\x1b[1;1f"
#define CLEAR_SCREEN          "\x1b[2J"

#define COLOR(x)              "\x1b["#x"m"

/* Not usable. Reference only. */
#define RESET                 0
#define BOLD                  1
#define FAINT                 2
#define ITALIC                3
#define UNDERLINE             4

#define FG_RED                31
#define FG_GREEN              32
#define FG_YELLOW             33
#define FG_BLUE               34
#define FG_MAGENTA            35
#define FG_CYAN               36
#define FG_WHITE              37
#define FG_DEFAULT            39

#define BG_RED                41
#define BG_GREEN              42
#define BG_YELLOW             43
#define BG_BLUE               44
#define BG_MAGENTA            45
#define BG_CYAN               46
#define BG_WHITE              47
#define BG_DEFAULT            49

/**** types ****/
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

/**** global data ****/
struct termios saved_terminal_attributes_;

/**** functions declarations ****/
// подбор кортежа B исходя из A формата width
value_t brute_force(value_t A, int width);
// шансы B перед A, ширина кортежей = width
double conway(value_t A, value_t B, int width);
// число (не пойми что) AB, ширина = width
value_t conway_single(value_t A, value_t B, int width);

// выдать случайный бит
value_t random_bit(void);
// выдать случайное число
value_t random_value(int width);
// встряхнуть генератор
void random_init(void);

// инициализация структуры кортежа форматом bc
void bg_init(Bit_generator_t *bg, size_t bc);
// затолкнуть следующий бит в кортеж
const Bits_t *bg_next_bit(Bit_generator_t *bg);

// ожидание кнопки и ее выдача
int keypress(void);
// вывод
void output(value_t A, value_t B, const Bits_t *bits, int width);

void reset_input_mode(void);
void setup_terminal(void);

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
       (!(conway(A, B, width) > 0.5)) && (B < limit);
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

value_t random_bit(void)
{
  return random() > (RAND_MAX >> 0x01);
}

value_t random_value(int width)
{
  return random() & ((1 << width) - 0x01);
}

void random_init(void)
{
  srandom(time(NULL));
}

inline
value_t fetch_bit(value_t V, size_t bit) {
  return V & (0x01 << bit);
}

void output(value_t A, value_t B, const Bits_t *bits, int width)
{
  ssize_t i;
  value_t bit, bit_v;
  value_t value = bits->value;
  size_t bc = bits->bitCount;
  ssize_t equal_length;

  printf(CLEAR_SCREEN);
  printf(CURSOR_TO_UL);

  printf(COLOR(31) "A MSB ");
  for (i = width - 1, equal_length = 0; i > -1; --i) {
    bit = fetch_bit(A, i);
    printf("%1d ", (int)(bit != 0));
#ifdef USE_BOLD_FONT
    if (equal_length != -1 && bc == width) {
      bit_v = fetch_bit(value, i);
      if (bit == bit_v) {
        ++equal_length;
        printf(COLOR(1) "%1d " COLOR(31), (int)(bit != 0));
      } else {
        equal_length = -1;
        printf("%1d ", (int)(bit != 0));
      }
    }
    else {
      printf("%1d ", (int)(bit != 0));
    }
#endif
  }
  printf("LSB" COLOR(0) "\n");

  printf("      ");
  for (i = bc - 1; i > -1; --i) {
    bit = fetch_bit(value, i);
    printf("%1d ", (int)(bit != 1));
  }
  printf("\n");

  printf(COLOR(32) "B MSB ");
  for (i = width - 1, equal_length = 0; i > -1; --i) {
    bit = fetch_bit(B, i);
    printf("%1d ", (int)(bit != 0));
#ifdef USE_BOLD_FONT
    if (equal_length != -1 && bc == width) {
      bit_v = fetch_bit(value, i);
      if (bit == bit_v) {
        ++equal_length;
        printf(COLOR(1) "%1d " COLOR(31), (int)(bit != 0));
      } else {
        equal_length = -1;
        printf("%1d ", (int)(bit != 0));
      }
    }
    else {
      printf("%1d ", (int)(bit != 0));
    }
#endif
  }
  printf("LSB" COLOR(0) "\n");
}

int keypress()
{
  char c;
  read(STDIN_FILENO, &c, 1);
  return c;
/********************
  int k;
  scanf("%d", &k);
  return k;
*********************/
}

void at_exit(void)
{
  printf("Exiting\n");
}

void reset_input_mode(void)
{
  tcsetattr(STDIN_FILENO, TCSANOW, &saved_terminal_attributes_);
}

void setup_terminal(void)
{
  struct termios term_attr;
  char *name;

  if (!isatty(STDIN_FILENO)) {
    fprintf(stderr, "stdin is not terminal");
    exit(EXIT_FAILURE);
  }

  tcgetattr (STDIN_FILENO, &saved_terminal_attributes_);
  atexit (reset_input_mode);
  tcgetattr (STDIN_FILENO, &term_attr);
  term_attr.c_lflag &= ~(ICANON|ECHO);
  term_attr.c_cc[VMIN] = 1;
  term_attr.c_cc[VTIME] = 0;
  tcsetattr (STDIN_FILENO, TCSAFLUSH, &term_attr);
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
  char *end;

  if (argc != 2) {
    printf("usage: %s width\n", argv[0] ? argv[0] : "n/a");
    exit(EXIT_SUCCESS);
  }

  atexit(at_exit);

  setup_terminal();

  width = strtol(argv[1], &end, 10);
  if (end == argv[1]) {
    fprintf(stderr, "'%s' is not a number\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  random_init();

  do {
    A = random_value(width);
    B = brute_force(A, width);

    bg_init(&bg, width);

    do {
      bits = bg_next_bit(&bg);
      output(A, B, bits, width);

      if (bits->bitCount == width && A == bits->value) {
        printf("A wins\n");
        break;
      }
      if (bits->bitCount == width && B == bits->value) {
        printf("B wins\n");
        break;
      }

      printf("ESC = stop\n");
      kp = keypress();
    } while (kp != ESC);

    printf("ESC = exit\n");
    kp = keypress();
  } while (kp != ESC);

  return EXIT_SUCCESS;
}
