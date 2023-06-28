CC=gcc
SRC=src/
BIN=bin/

helix: $(SRC)helix.c
	$(CC) $(SRC)helix.c -o $(BIN)helix -Wall -Wextra -pedantic -std=c99

clean:
	rm -f $(BIN)*
