# e

Braindead editor. Feels like Vim, only simpler. Inspired by
[kilo](https://github.com/antirez/kilo), of course, and a large
swath of the code is similar.

## Features

- Incremental search (and replace)
- Multiple modi (similar to Vim)
- Mnemonic movement (feels like Vim, just different enough for you to be frustrated)
- Limitless Undo (until memory runs out)
- Extensible syntax highlighting
- No global state in the library part (just in `main.c`)
- Can be used as a library

## Installation

```
git clone https://github.com/hellerve/e
cd e
make install
```

## Usage

There are two major modes, `init` and `edit`. `edit` mode works like a normal
text editor would. `init` mode enables the user to navigate and do meta work,
such as saving the file, searching, and replacing.

<hr/>

Have fun!
