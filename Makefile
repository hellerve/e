TARGET=e
BUILDDIR=bin/
STXDIR=~/.estx
PREFIX=/usr/local/bin/
SOURCES=$(wildcard src/*.c)
MAIN=main.c
override CFLAGS+=-Werror -Wall -g -fPIC -O2 -DNDEBUG -ftrapv -Wfloat-equal -Wundef -Wwrite-strings -Wuninitialized -pedantic

all: main.c
	mkdir -p $(BUILDDIR)
	$(CC) $(MAIN) $(SOURCES) -DSTXDIR=\"$(STXDIR)\" -o $(BUILDDIR)$(TARGET) $(CFLAGS)

stx:
	mkdir -p $(STXDIR)
	cp stx/* $(STXDIR)

install: all stx
	install $(BUILDDIR)$(TARGET) $(PREFIX)$(TARGET)

uninstall:
	rm -rf $(PREFIX)$(TARGET)
