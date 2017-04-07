all: main.c
	$(CC) main.c src/*.c -o e -Wall -Wextra -pedantic -std=c99 -O2
