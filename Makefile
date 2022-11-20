CC := clang
CFLAGS := -g -Wall -Wno-deprecated-declarations -Werror -fsanitize=address

all: editor

clean:
	rm -f editor

editor: editor.c rope.c rope.h
	$(CC) $(CFLAGS) -o editor editor.c rope.c -lncurses

format:
	@echo "Reformatting source code."
	@clang-format -i --style=file $(wildcard *.c) $(wildcard *.h)
	@echo "Done."

.PHONY: all clean format
