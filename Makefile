TARGET=e
BUILDDIR=bin/
STXDIR=$(HOME)/.estx
ERC=$(HOME)/.erc
PREFIX=/usr/local/bin/
SOURCES=$(wildcard src/*.c)
MAIN=main.c
override CFLAGS+=-lexecinfo -Werror -Wall -g -fPIC -O2 -DNDEBUG -ftrapv -Wfloat-equal -Wundef -Wwrite-strings -Wuninitialized -pedantic -std=c11

all: main.c syntax
	mkdir -p $(BUILDDIR)
	$(CC) $(MAIN) $(SOURCES) -DSTXDIR=\"$(STXDIR)\" -o $(BUILDDIR)$(TARGET) $(CFLAGS)

lua:
	make all CFLAGS+="-DWITH_LUA -L./vendor/lua-5.3.4/src -llua"

syntax:
	mkdir -p $(STXDIR)
	cp stx/* $(STXDIR)

install: all
	install $(BUILDDIR)$(TARGET) $(PREFIX)$(TARGET)

install_lua:
	make install CFLAGS+="-DWITH_LUA -L./vendor/lua-5.3.4/src -llua -DERC=\"$(ERC)\""
	touch ~/.erc

uninstall:
	rm -rf $(PREFIX)$(TARGET)
