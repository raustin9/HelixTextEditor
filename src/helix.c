/* INCLUDES */
#include <asm-generic/ioctls.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

/* DEFINES */
#define CTRL_KEY(k) ((k) & 0x1f) // makes the first 3 bits of character 0 to make it ctrl-key

/* DATA */
// struct termios orig_termios;

// Gobal State
struct EditorConfig {
  int screenrows;
  int screencols;
  struct termios orig_termios;
};

struct EditorConfig E;

/* TERMINAL */

// Perform error handling actions
// and kill process
void
Die(const char* s) {
  write(STDOUT_FILENO, "\x1b[2J", 4); // clears the screen
  write(STDOUT_FILENO, "\x1b[H", 3);  // reposition cursor at top left of screen to begin drawing
  
  perror(s);
  exit(1);
}

// Reset the terminal attributes
// to the original values 
// Used when the program exits
void
DisableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) {
    Die("tcsetattr");
  }
}

// Disable canonical mode in the terminal
// and enable 'raw' mode so that we can 
// easily draw to the screen and read inputs
void
EnableRawMode() {
  struct termios raw;

  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) {
    Die("tcgetattr");
  }
  atexit(DisableRawMode);

  raw = E.orig_termios;
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

// Read a key from 
// user input
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

int
GetCursorPosition(int *rows, int *cols) {
  char buf[32];
  uint32_t i;

  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

  i = 0;
  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

  return 0;
}

// Get the dimensions of 
// the terminal window
int
GetWindowSize(int *rows, int *cols) {
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    // Move the cursor to the bottom right of screen
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
      return -1;
    }
    return GetCursorPosition(rows, cols);

  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/* OUTPUT */

// Draw the rows to the screen
void
EditorDrawRows() {
  int y;

  for (y = 0; y < E.screenrows; y++) {
    write(STDOUT_FILENO, "$", 1);

    if (y < E.screenrows -1) {
      write(STDOUT_FILENO, "\r\n", 2);
    }
  }
}

// Clear the screen and perform other
// actions to draw to the screen
void
EditorRefreshScreen() {
  write(STDOUT_FILENO, "\x1b[2J", 4); // clears the screen
  write(STDOUT_FILENO, "\x1b[H", 3);  // reposition cursor at top left of screen to begin drawing
  
  EditorDrawRows();

  write(STDOUT_FILENO, "\x1b[H", 3);
}

/* INPUT */

// Process the logic for which key 
// was pressed
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

// Initialize the editor
void
InitEditor() {
  if (GetWindowSize(&E.screenrows, &E.screencols) == -1) {
    Die("Get window size");
  }
}

// Main function
int
main(int argc, char** argv) {
  EnableRawMode();
  InitEditor();

  while (1) {
    EditorRefreshScreen();
    EditorProcessKeypress();
  }

  return 0;
}
