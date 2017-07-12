# e

Braindead editor. Feels like Vim, only simpler. Inspired by
[kilo](https://github.com/antirez/kilo), of course, and a large
swath of the code is similar.

It can be scripted through Lua.

[![asciicast](https://asciinema.org/a/e164s5tnu3okht44go6uhyju4.png)](https://asciinema.org/a/e164s5tnu3okht44go6uhyju4)

## Features

- Scripting through intuitive Lua interface
- Incremental search (and replace)
- Multiple modi (similar to Vim)
- Mnemonic movement (feels like Vim, just different enough for you to be frustrated)
- Limitless Undo (until memory runs out)
- Extensible syntax highlighting
- No global state in the library part (just in `main.c`)
- Can be used as a library
- Ships with syntax highlighting for
  - C/C++ (stable)
  - Python (experimental)
  - JavaScript (experimental)
  - Go (experimental)
  - Haskell (experimental)
  - Markdown (unfinished)

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

### init mode mnemonics

Use `wasd` or the arrow keys for movement. Editing (backspace, etc.) works normally.

- `n`: insert a line below the cursor and start editing (*n*ext) 
- `p`: insert a line above the cursor and start editing (*p*revious)
- `b`: jump to the *b*eginning of the line and start editing
- `t`: jump to the end of the line and start editing (*t*erminus)
- `h`: *h*ide a line (delete it) and add it to the system clipboard (clipboard only on Windows and OS X)
- `c`: *c*opy a line to the system clipboard (only on Windows and OS X)
- `v`: *v*iew (i.e. paste) the contents of the system clipboard (only on Windows and OS X)
- `/`: incremental highlighted search
- `r`: search and replace first occurrence
- `R`: search and replace all occurrences
- Space: quick save (might be prompted for a file name)

In meta mode (reachable by pressing the colon character `:`), there are
the following commands:

- `s`: save and quit (might be prompted for a file name)
- `q`: exit (will abort if the file has unsaved content)
- `!`: force exit
- Number `n`: jump to line `n`

### Writing syntax files

By default, `e` creates a directory called `.estx` in the user's home
directory (the location is overridable by providing `STXDIR` to `make install`).
There, `e` will search for syntax files on startup. Their grammar is very
minimal, see the C file below:

```
displayname: c
extensions: .*\.cpp$
            .*\.hpp$
            .*\.c$
            .*\.h$
comment|no_sep: //.*$
keyword:(restrict|switch|if|while|for|break|continue|return|else|try|catch|else|struct|union|class|typedef|static|enum|case|asm|default|delete|do|explicit|export|extern|inline|namespace|new|public|private|protected|sizeof|template|this|typedef|typeid|typename|using|virtual|friend|goto)
type: (auto|bool|char|const|double|float|inline|int|mutable|register|short|unsigned|volatile|void|int8_t|int16_t|int32_t|int64_t|uint8_t|uint16_t|uint32_t|uint64_t|size_t|ssize_t|time_t)
comment|no_sep: /\*([^(\*/)]*)?
                 (.*)?\*/
pragma: #(include|pragma|define|undef) .*$
predefined: (NULL|stdout|stderr)
pragma: #(ifdef|ifndef|if) .*$
             #endif\w*$
```

`displayname` is the string displayed at the bottom of `e`. `extensions`
is a list of regexes to match the filenames. Highlighting keys are `comment`,
`keyword`, `type`, `pragma`, `string`, `number`, and `predefined`. By appending
`|no_sep`, the user signals to `e` that no separator is needed, i.e. highlighting
works even if the matched string is part of a longer word. The values are regexes.

If you provide a second regex (must by divided by a newline), `e` assumes that everything
between the two matches should be colored (useful for e.g. multiline comments).

### Scripting through Lua

The editor has scripting capabilities in Lua. Thus far I've only documented them
in [a blog post](http://blog.veitheller.de/Editing_Revisited.html), but this
post should give you a good overview of how to write Lua scripts for `e`. There
is also an example [`.erc`](https://github.com/hellerve/e/blob/master/.erc)
file in the repository that you can look at for inspiration.

That's it!

<hr/>

Have fun!
