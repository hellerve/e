all: src/e.c
	$(CC) src/*.c -o e -Wall -Wextra -pedantic -std=c99
