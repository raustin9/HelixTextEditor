/* INCLUDES */
#include <asm-generic/ioctls.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

/* DEFINES */
#define CTRL_KEY(k) ((k) & 0x1f) // makes the first 3 bits of character 0 to make it ctrl-key
#define HELIX_VERSION "0.0.1"

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


/* APPEND BUFFER */

// Structure for the append buffer
// This will allow us to have dynamic strings
// that can be appended
struct abuf {
  char *b;
  int len;
};

#define ABUF_INIT {NULL, 0}

// Append a string to the AppendBuffer's buffer
void
AbAppend(struct abuf *ab, const char *s, int len) {
  char *new;

  new = (char*)realloc(ab->b, ab->len + len);

  if (new == NULL) return;

  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

// Destructor for the append buffer class
void
AbFree(struct abuf *ab) {
  free(ab->b);
}

/* OUTPUT */

// Draw the rows to the screen
void
EditorDrawRows(struct abuf *ab) {
  int y;

  for (y = 0; y < E.screenrows; y++) {
    if (y == E.screenrows / 3) {
      char welcome[80];
      int welcomelen = snprintf(welcome, sizeof(welcome),
          "Helix Editor -- %s", HELIX_VERSION);

      if (welcomelen > E.screencols) welcomelen = E.screencols;

      int padding = (E.screencols - welcomelen) / 2;
      if (padding) {
        AbAppend(ab, "$", 1);
        padding--;
      }
      while (padding--) AbAppend(ab, " ", 1);

      AbAppend(ab, welcome, welcomelen);
    } else {
      AbAppend(ab, "$", 1);
    }

    AbAppend(ab, "\x1b[K", 3); // erase part of the line to the right of the cursor
    if (y < E.screenrows -1) {
      AbAppend(ab, "\r\n", 2);
    }
  }
}

// Clear the screen and perform other
// actions to draw to the screen
void
EditorRefreshScreen() {
  struct abuf ab = ABUF_INIT;

  AbAppend(&ab, "\x1b[?25l", 6); // hide the cursor 
  // AbAppend(&ab, "\x1b[2J", 4);   // clear the screen
  AbAppend(&ab, "\x1b[H", 3);    // move cursor to top left of terminal
  
  EditorDrawRows(&ab);

  AbAppend(&ab, "\x1b[H", 3); 
  AbAppend(&ab, "\x1b[?25h", 6); // show the cursor 

  write(STDOUT_FILENO, ab.b, ab.len);
  AbFree(&ab);
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
