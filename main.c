#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>

#if 0
#define LogPrint(fmt,...) do { fprintf(stderr, fmt "\n", ##__VA_ARGS__); } while(0)
#else
#define LogPrint(fmt, ...)
#endif

#define ESC                   '\033'
#define END_OF_FILE           '\004'
#define ESC_PREFIX            "\x1b["
#define ESC_DELIMITER         ";"

#define COLOR_SUFFIX          "m"
#define CURSOR_POSITION_SUFIX "f"
#define CURSOR_FORWARD_SUFFIX "C"
#define CURSOR_DOWN_SUFFIX    "B"
#define CURSOR_UP_SUFFIX      "A"
#define CURSOR_BACK_SUFFIX    "D"

#define SAVE_CURSOR_POSITION  "\x1b[s"
#define REST_CURSOR_POSITION  "\x1b[u"

#define CURSOR_TO_UL          "\x1b[1;1f"
#define CURSOR_TO_LINE_START  "\x1b[1G"

#define CLEAR_SCREEN          "\x1b[2J"
#define CLEAR_LINE            "\x1b[2K"
#define CLEAR_LINE_BEFORE     "\x1b[1K"

#define COLOR(x)              "\x1b["#x"m"

#define NEW_LINE              "\n"

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
  size_t bits_count;
} Bits_t;
typedef struct {
  Bits_t bits;
  value_t mask;
  size_t bits_countMax;
} Bit_generator_t;

/**** global data ****/
struct termios saved_terminal_attributes_;
struct winsize output_terminal_winsize_;
static const char BIT_CHAR[2] = {
  [0] = '0',
  [1] = '1'
};

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
// вывод только одной строки с префиксом, заданного цвета,
// и некотоорым кол-вом выделенных битов (начиная от старшего)
// с заданным форматом и доведением до минимальной ширины (если width < min_width)
// положение по горизонтали = shift
void output_single(char prefix, int color, int bold_count, int shift, value_t value, int width, int min_width);
// вывод
void output(value_t A, value_t B, int bold_A, int bold_B, const Bits_t *bits, int width);
// вывести ведущее число. выдает - длину выведенной строки.
int output_lead_number(const char *prefix, int shift, value_t LN, int width);

void output_chances(const char *prefix, int shift, value_t num, value_t denom);

// сравнить заданное начение с тестовым кортежем.
// На выходе - длина равных битов начиная от старшего
int compare(value_t value, int value_width, const Bits_t *bits);

void reset_input_mode(void);
void setup_input_terminal(void);
void setup_output_terminal(void);

/**** functions definitions ****/
value_t conway_single(value_t A, value_t B, int width)
{
  int k;
  value_t maskA = (1 << width) - 1;
  value_t A_, B_;
  value_t ret;

  for (k = 0, A_ = A, B_ = B, ret = 0;
       k < width;
       maskA >>= 0x01, A_ = (A & maskA), B_ >>= 1, ++k) {
    ret = (ret << 0x01) | (A_ == B_);
    LogPrint("  [%04x]: maskA: %04x, A_: %04x, B_: %04x, bit: %x, ret: %04x",
             k, maskA, A_, B_, (A_ == B_), ret);
  }

  LogPrint("-- %#x", ret);

  return ret;
}

double conway(value_t A, value_t B, int width)
{
  value_t AA, AB, BB, BA;
  double num, denom;
  LogPrint("AA:");
  AA = conway_single(A, A, width);
  LogPrint("AB:");
  AB = conway_single(A, B, width);
  LogPrint("BB:");
  BB = conway_single(B, B, width);
  LogPrint("BA:");
  BA = conway_single(B, A, width);

  num = AA - AB;
  denom = BB - BA;

  return num / denom;
}

value_t brute_force(value_t A, int width)
{
  value_t limit = 1 << width;
  value_t B = 0;
  double c = conway(A, B, width);

  for (B = 0;
       (!(c > 1)) && (B < limit);
       ++B, c = conway(A, B, width)) {
    LogPrint("A: %#0x, B: %#0x, c: %.6f", (int)A, (int)B, c);
  }

  LogPrint("A: %#0x, B: %#0x, c: %.6f", (int)A, (int)B, c);

  return B;
}

void bg_init(Bit_generator_t *bg, size_t bc)
{
  memset(bg, 0, sizeof(Bit_generator_t));
  bg->bits_countMax = bc;
  bg->mask = (0x01 << bc) - 0x01;
}

const Bits_t *bg_next_bit(Bit_generator_t *bg)
{
  Bits_t *bits = &bg->bits;
  value_t ls_bit = random_bit();
  bits->value = ((bits->value << 0x01) | ls_bit) & bg->mask;
  if (bits->bits_count < bg->bits_countMax) ++bits->bits_count;

  return bits;
}

value_t random_bit(void)
{
  return random() & 0x01;// > (RAND_MAX >> 0x01);
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
value_t fetch_bit(value_t V, size_t bit)
{
  return ((V & (0x01 << bit)) != 0);
}

int compare(value_t value, int value_width, const Bits_t *bits)
{
  int idx;
  int length;
  size_t bc = bits->bits_count;
  value_t bv = bits->value;
  value_t bit_value, bit_test;

  for (idx = 0, length = 0; idx < bc; ++idx) {
    bit_value = fetch_bit(value, value_width - idx - 1);
    bit_test = fetch_bit(bv, bc - idx - 1);

    if (bit_value == bit_test) {
      ++length;
      continue;
    }

    break;
  }

  return length;
}

void output_single(char prefix, int color, int bold_count, int shift, value_t value, int width, int min_width)
{
  ssize_t idx;
  value_t bit;
  char ch;

  printf(ESC_PREFIX "%d" CURSOR_FORWARD_SUFFIX, shift);
  printf(ESC_PREFIX "%d" COLOR_SUFFIX "%c MSB ", color, prefix);

  printf(ESC_PREFIX "%d" COLOR_SUFFIX, BOLD);
  for (idx = width - 1; idx > width - 1 - bold_count; --idx) {
    bit = fetch_bit(value, idx);
    ch = BIT_CHAR[bit];
    printf("%c ", ch);
  }

  printf(ESC_PREFIX "%d" COLOR_SUFFIX, RESET);
  printf(ESC_PREFIX "%d" COLOR_SUFFIX, color);

  for (; idx > -1; --idx) {
    bit = fetch_bit(value, idx);
    ch = BIT_CHAR[bit];
    printf("%c ", ch);
  }

  for (idx = width; idx < min_width; ++idx) {
    printf("  ");
  }

  printf("LSB" ESC_PREFIX "%d" COLOR_SUFFIX, RESET);
}

void output(value_t A, value_t B, int bold_A, int bold_B, const Bits_t *bits, int width)
{
  int horiz_shift = output_terminal_winsize_.ws_col / 4;
  /*
    printf(ESC_PREFIX "%d" ESC_DELIMITER "%d" CURSOR_POSITION_SUFIX,
           output_terminal_winsize_.ws_row / 4,
           1);
  */
  output_single('A', FG_RED, bold_A, horiz_shift, A, width, width);
  printf(NEW_LINE);
  output_single(' ', RESET, 0, horiz_shift, bits->value, bits->bits_count, width);
  printf(" <== next bit" NEW_LINE);
  output_single('B', FG_GREEN, bold_B, horiz_shift, B, width, width);
  printf(NEW_LINE);
}

int output_lead_number(const char *prefix, int shift, value_t LN, int width)
{
  int idx;
  char ch;
  value_t bit;

  printf(ESC_PREFIX "%d" CURSOR_FORWARD_SUFFIX, shift);
  printf("%s MSB ", prefix);

  for (idx = width - 1; idx > -1; --idx) {
    bit = fetch_bit(LN, idx);
    ch = BIT_CHAR[bit];
    printf("%c ", ch);
  }

  printf("LSB");

  return strlen(prefix) + 5 + (width * 2) + 3;
}

void output_chances(const char *prefix, int shift, value_t num, value_t denom)
{
  printf(ESC_PREFIX "%d" CURSOR_FORWARD_SUFFIX "%s %lu / %lu",
         shift, prefix, (unsigned long)num, (unsigned long)denom);
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
  printf(ESC_PREFIX "%d" ESC_DELIMITER "%d" CURSOR_POSITION_SUFIX,
         output_terminal_winsize_.ws_row - 1, 1);
  printf("Exiting\n");
}

void reset_input_mode(void)
{
  tcsetattr(STDIN_FILENO, TCSANOW, &saved_terminal_attributes_);
}

void setup_input_terminal(void)
{
  struct termios term_attr;
  char *name;

  if (!isatty(STDIN_FILENO)) {
    fprintf(stderr, "stdin is not terminal\n");
    exit(EXIT_FAILURE);
  }

  tcgetattr (STDIN_FILENO, &saved_terminal_attributes_);
  atexit (reset_input_mode);
  tcgetattr (STDIN_FILENO, &term_attr);
  term_attr.c_lflag &= ~(ICANON | ECHO);
  term_attr.c_cc[VMIN] = 1;
  term_attr.c_cc[VTIME] = 0;
  tcsetattr (STDIN_FILENO, TCSAFLUSH, &term_attr);
}

void setup_output_terminal(void)
{
  if (!isatty(STDOUT_FILENO)) {
    fprintf(stderr, "stdout is not terminal\n");
    exit(EXIT_FAILURE);
  }

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &output_terminal_winsize_)) {
    perror("ioctl(stdout)");
    exit(EXIT_FAILURE);
  }

  if (setvbuf(stdout, NULL, _IONBF, 0)) {
    perror("setvbuf(_IONBUF)");
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char **argv)
{
  /* usage: argv[0] <width> */
  value_t A;
  value_t B;
  value_t AA, AB, BB, BA;
  value_t chances_num, chances_denom;
  size_t score_A = 0, score_B = 0;
  int kp;
  int width;
  int lead_number_line_len;
  int len_A, len_B;
  Bit_generator_t bg;
  const Bits_t *bits;
  char *end;

  if (argc != 2) {
    printf("bit width = ");
    scanf("%d", &width);
#if 0
    printf("usage: %s width\n", argv[0] ? argv[0] : "n/a");
    exit(EXIT_SUCCESS);
#endif
  } else {
    width = strtol(argv[1], &end, 10);
    if (end == argv[1]) {
      fprintf(stderr, "'%s' is not a number\n", argv[1]);
      exit(EXIT_FAILURE);
    }
  }

  atexit(at_exit);

  setup_output_terminal();
  setup_input_terminal();

  printf(CLEAR_SCREEN);

  random_init();
#if 0
  width = 4;
  A = 0x0b;
  B = brute_force(A, width);
  LogPrint("A: %#x, B: %#x", A, B);
#endif

  A = random_value(width);
  B = brute_force(A, width);

  AA = conway_single(A, A, width);
  AB = conway_single(A, B, width);
  BB = conway_single(B, B, width);
  BA = conway_single(B, A, width);

  chances_num = AA - AB;
  chances_denom = BB - BA;

  do {
    printf(CURSOR_TO_UL);
    printf(ESC_PREFIX "%d" ESC_DELIMITER "%d" CURSOR_POSITION_SUFIX,
           output_terminal_winsize_.ws_row / 8, 1);

    lead_number_line_len = output_lead_number("AA", output_terminal_winsize_.ws_col / 8, AA, width);
    printf(NEW_LINE);
    output_lead_number("AB", output_terminal_winsize_.ws_col / 8, AB, width);

    printf(ESC_PREFIX "%d" CURSOR_UP_SUFFIX, 1);
    printf(CURSOR_TO_LINE_START);

    output_lead_number("BB", output_terminal_winsize_.ws_col / 8 + lead_number_line_len + 3, BB, width);
    printf(NEW_LINE);
    output_lead_number("BA", output_terminal_winsize_.ws_col / 8 + lead_number_line_len + 3, BA, width);
    printf(NEW_LINE);

    printf(NEW_LINE);
    printf(CURSOR_TO_LINE_START);
    output_chances("chances B-A:", output_terminal_winsize_.ws_col * 3 / 16, chances_num, chances_denom);
    printf(NEW_LINE);
    output_chances("score B / A:", output_terminal_winsize_.ws_col * 3 / 16, score_B, score_A);
    printf(NEW_LINE);

    printf(CURSOR_TO_LINE_START);
    printf(ESC_PREFIX "%d" CURSOR_DOWN_SUFFIX, output_terminal_winsize_.ws_row / 8);

    printf(SAVE_CURSOR_POSITION);

    bg_init(&bg, width);

    do {
      printf(REST_CURSOR_POSITION);

      bits = bg_next_bit(&bg);

      len_A = compare(A, width, bits);
      len_B = compare(B, width, bits);

      output(A, B, len_A, len_B, bits, width);

      printf(CLEAR_LINE);
      if (len_A == width) {
        ++score_A;
        printf("Finish: A\n");
        break;
      }
      if (len_B == width) {
        ++score_B;
        printf("Finish: B\n");
        break;
      }

      printf("ESC = stop");
      kp = keypress();
      printf(CLEAR_LINE_BEFORE);
    } while (kp != ESC);

    printf("ESC = exit");
    kp = keypress();
    printf(CLEAR_LINE_BEFORE);
  } while (kp != ESC);

  return EXIT_SUCCESS;
}
