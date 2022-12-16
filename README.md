# buffalo: a terminal text editor 🦬

> *Buffalo buffalo Buffalo buffalo buffalo buffalo Buffalo buffalo.*

**buffalo** is a terminal text editor written in C and built on top of ncurses.

![preview](resources/buffalo-preview.png)

## Usage

Compile the program with `make`, dependencies are the `ncurses` and `pthread` libraries. Then run it by passing the name of a file as and argument. The file will be created if it does not already exist:

```sh
$ make
$ ./buffalo hello.txt
```

The editor can be navigated using the arrow keys or scrolled with the mouse.

## Configuration

The editor can be configured with a `.buffalorc` file. The editor first checks for such a file in the current directory, and if not found it looks in the home directory. The configuration file has a simple key/value structure. Here's an example:

```
build: make
test: make test
```

This configures the editor to use `make` as the build command and `make test` as the test command.

## Shortcuts

The following shortcuts are recognized:

```
Ctrl+S: save
Ctrl+Q: quit
Ctrl+B: build
Ctrl+T: test
```

## Limitations

There are many unimplemented features:

- Horizontal scrolling has not been implemented, so characters past the width of the terminal cannot be viewed, but they can be edited.
- Tab input has not been implemented

## Acknowledgements

This project was built as my final project for CSC-213: Operating Systems at Grinnell College. Some of the initial code was borrowed from [Charlie Curtsinger](https://curtsinger.cs.grinnell.edu/). Slides for my presentation can be found under `resources`.