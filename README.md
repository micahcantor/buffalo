# buffalo: a terminal text editor

**buffalo** is a terminal text editor written in C and built on top of ncurses.

## Running

Compile the program with `make`, then run it by passing the name of a file as and argument:

```sh
$ make
$ ./buffalo hello.txt
```

## Configuration

The editor can be configured with a `.buffalorc` file. The editor first checks for such a file in the current directory, and if not found it looks in the home directory. The configuration file has a simple key/value structure. Here's an example:

```
build: make
test: make test
```

This configures the editor to use `make` as the build command and `make test` as the test command.

## Bugs

There are several known bugs:

- Adding characters on a current line past the width of the terminal causes a segfault.