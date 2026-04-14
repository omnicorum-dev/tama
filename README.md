# tama

A minimal command-line task manager. Tasks are stored as plain text files on disk so you don't have to rely on github.

---

## Building

**Requirements**

- C++17 compiler (GCC 9+, Clang 10+, or MSVC 2019+)
- CMake 3.15+ (or build manually)

**With CMake**

```sh
git clone https://github.com/omnicorum-dev/tama
cd tama
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The binary ends up at `build/tama`. Copy it somewhere on your `PATH`, or symlink it so you can pull any updates and have them automatically reflect:

```sh
# copy to path
cp build/tama /usr/local/bin/tama

# symlink
ln -s build/tama /usr/local/bin/tama
```

**Without CMake**

```sh
g++ -std=c++17 -O2 -o tama main.cpp
```

> `base.h` must be on the include path. If it lives next to `main.cpp` the command above is sufficient. Adjust `-I` as needed.

---

## Getting started

Navigate to the root of a project (or anywhere you want tasks to live) and initialise:

```sh
tama init
```

This creates a `TASKS/` directory in the current folder. All subsequent commands search upward through parent directories to find it, so you can run `tama` from any subdirectory of your project.

---

## Commands

### `init`
Create a `TASKS/` folder in the current directory.
```sh
tama init
```

### `new`
Create a task interactively. The UID is derived from the current timestamp (`YYYYMMDDHHmmss`).
```sh
tama new
# Task name: Fix login bug
# Task priority (integer): 10
# Enter tags ('end' when completed)
# bug
# auth
# end
# Task description: Users are getting 401 on valid tokens
```

### `ls [expression]`
List tasks sorted by priority (highest first). With no expression, all tasks are shown. Pass a boolean filter expression to narrow results.

```sh
tama ls                     # all tasks
tama ls open                # open tasks only
tama ls open and bug        # open tasks tagged 'bug'
tama ls not closed          # same as 'open'
tama ls bug or urgent       # tasks tagged 'bug' or 'urgent'
tama ls open and not low    # open tasks without the 'low' tag
```

**Filter expression reference**

Operands are tag names, `open`, or `closed`. Operators in order of precedence (highest first):

| Operator | Meaning |
|----------|---------|
| `not` | logical NOT (unary) |
| `nand` | NOT AND |
| `and` | logical AND |
| `xor` | exclusive OR |
| `xnor` | exclusive NOR |
| `or` | logical OR |
| `nor` | NOR |
| `impl` | implication (A → B) |
| `( )` | grouping |

A bare operand with no operator is implicitly `not` when it appears at the start of an expression or after another operand. Parentheses can override precedence as expected.

### `more <uid>`
Show full details for a task including its description.
```sh
tama more 20240413103045
```

### `edit <uid>`
Edit a task interactively. Press Enter to keep the current value for any field.
```sh
tama edit 20240413103045
# Task name [Fix login bug]:
# Task priority [10]: 20
# Current tags: bug, auth
# Enter new tags ('end' when done, leave blank to keep current):
# (blank — tags unchanged)
# end
# Task description [Users are getting 401 on valid tokens]:
```

### `close <uid>` / `open <uid>`
Toggle a task's status.
```sh
tama close 20240413103045
tama open  20240413103045
```

### `tag <uid> <tag>` / `untag <uid> <tag>`
Add or remove a single tag without touching anything else.
```sh
tama tag   20240413103045 urgent
tama untag 20240413103045 low
```

### `status <uid> <open|closed>`
Set status explicitly. Prefer `open` / `close` for interactive use; `status` is useful in scripts.
```sh
tama status 20240413103045 closed
```

### `find`
Print the path of the `TASKS/` folder tama would use from the current directory.
```sh
tama find
# Tasks folder: /home/user/projects/myapp/TASKS
```

---

## File format

Each task is a directory under `TASKS/`:

```
TASKS/
└── 20240413103045/
    └── task.txt
```

`task.txt` is a plain text file:

```
20240413103045          ← UID (line 1)
Fix login bug           ← name (line 2)
10                      ← priority (line 3)
open                    ← status: "open" or "closed" (line 4)
bug,auth                ← comma-separated tags, no spaces (line 5)
                        ← blank line (line 6)
Users are getting 401   ← description (remaining lines)
on valid tokens.
```

Because tasks are plain files you can edit them by hand, diff them in git, grep across them, or write your own tooling on top.
Additionally, you can put any related files in the folder next to the `task.txt` to keep everything together.

---

## Tips

**Track tasks in git**
```sh
git init
tama init
echo "TASKS/" >> .gitignore   # if you don't want to track them
# or just commit TASKS/ — each task gets a clean diff
```

**Filter by multiple tags**
```sh
tama ls open and bug and not wontfix
```

**Alias for quick listing**
```sh
alias tt='tama ls open'
```

---

## License

MIT
