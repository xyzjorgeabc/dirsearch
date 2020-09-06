#include <sys/ioctl.h>
#include <termios.h>
#include <locale.h>

struct termios term_attrs_def;
int size_x = 0;
int size_y = 0;

int get_win_size(int *x, int *y) {
  struct winsize w;
  int needs_update = 0;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  if (size_x != w.ws_col || size_y != w.ws_row) needs_update = 1;
  (*x) = size_x = w.ws_col;
  (*y) = size_y = w.ws_row;

  return needs_update;
}
void clear_term () {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
}

void reset_tr(){
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  write(STDOUT_FILENO, "\e[?25h",6);
  write(STDOUT_FILENO, "\033c",3); 
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_attrs_def);
}

void reset_cursor () {
  write(STDOUT_FILENO, "\x1b[H", 3);
}

void enter_raw_term () {
  struct termios term_attrs;
  printf("\e[?25l");
  //printf("\x1b[7l");
  clear_term();
  setvbuf(stdin, NULL, _IONBF, 0); 
  tcgetattr(STDIN_FILENO, &term_attrs_def);
  tcgetattr(STDIN_FILENO, &term_attrs);
  term_attrs.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);
  term_attrs.c_oflag &= ~(OPOST);
  term_attrs.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  term_attrs.c_cflag |= (CS8);
  term_attrs.c_cc[VTIME] = 0;
  term_attrs.c_cc[VMIN] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_attrs);
  setlocale(LC_CTYPE,"");
  atexit(reset_tr);
}
