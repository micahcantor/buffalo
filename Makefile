CC := clang
CFLAGS := -g -Wall -Wno-deprecated-declarations -Werror -O3 -fsanitize=address

all: buffalo

clean:
	rm -f buffalo

buffalo: buffalo.c row.c row.h ui.c ui.h buffalo_state.c buffalo_state.h config.c config.h
	$(CC) $(CFLAGS) -o buffalo buffalo.c buffalo_state.c row.c ui.c config.c -lncurses -lform -lpthread

format:
	@echo "Reformatting source code."
	@clang-format -i --style=file $(wildcard *.c) $(wildcard *.h)
	@echo "Done."

.PHONY: all clean format
