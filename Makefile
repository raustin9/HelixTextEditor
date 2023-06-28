CC=gcc

helix: helix.c
	$(CC) helix.c -o helix -Wall -Wextra -pedantic -std=c99
