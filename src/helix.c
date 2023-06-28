/* INCLUDES */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

/* DATA */
struct termios orig_termios;

/* TERMINAL */
void
Die(const char* s) {
  perror(s);
  exit(1);
}

void
DisableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
    Die("tcsetattr");
  }
}

void
EnableRawMode() {
  struct termios raw;

  if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
    Die("tcgetattr");
  }
  atexit(DisableRawMode);

  raw = orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8); // set 8 bits per byte
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN]  = 0; // sets minimum number of bytes needed before read() can return
  raw.c_cc[VTIME] = 1; // sets maximum amount of time to wait before read() returns

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
    Die("tcsetattr");
  }
}

/* INIT */
int
main(int argc, char** argv) {
  char c;

  EnableRawMode();

  while (1) {
    c = '\0';
    if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) {
      Die("read");
    }

    if (iscntrl(c)) {
      printf("%d\r\n", c);
    } else {
      printf("%d ('%c')\r\n", c, c);
    }
    if (c == 'q') break;
  }

  return 0;
}
