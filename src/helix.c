/* INCLUDES */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

/* DEFINES */
#define CTRL_KEY(k) ((k) & 0x1f) // makes the first 3 bits of character 0 to make it ctrl-key

/* DATA */
struct termios orig_termios;

/* TERMINAL */
void
Die(const char* s) {
  write(STDOUT_FILENO, "\x1b[2J", 4); // clears the screen
  write(STDOUT_FILENO, "\x1b[H", 3);  // reposition cursor at top left of screen to begin drawing
  
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

char
EditorReadKey() {
  int nread;
  char c;

  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) {
      Die("read");
    }
  }

  return c;
}

/* OUTPUT */
void
EditorDrawRows() {
  int y;

  for (y = 0; y < 24; y++) {
    write(STDOUT_FILENO, "$\r\n", 3);
  }
}

void
EditorRefreshScreen() {
  write(STDOUT_FILENO, "\x1b[2J", 4); // clears the screen
  write(STDOUT_FILENO, "\x1b[H", 3);  // reposition cursor at top left of screen to begin drawing
  
  EditorDrawRows();

  write(STDOUT_FILENO, "\x1b[H", 3);
}

/* INPUT */

void
EditorProcessKeypress() {
  char c;

  c = EditorReadKey();

  switch (c) {
    case CTRL_KEY('q'):
		  write(STDOUT_FILENO, "\x1b[2J", 4); // clears the screen
		  write(STDOUT_FILENO, "\x1b[H", 3);  // reposition cursor at top left of screen to begin drawing
      exit(0);
      break;
  }
}

/* INIT */
int
main(int argc, char** argv) {
  EnableRawMode();

  while (1) {
    EditorRefreshScreen();
    EditorProcessKeypress();
  }

  return 0;
}
