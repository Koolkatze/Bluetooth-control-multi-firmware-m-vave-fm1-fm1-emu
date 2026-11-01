CC ?= cc
CFLAGS ?= -std=c99 -O2 -Wall -Wextra -Werror -pedantic
.PHONY: test clean
test: urban_test urban_ui_test
	./urban_test
	./urban_ui_test
urban_test: urban_test.c urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_test.c
urban_ui_test: urban_ui_test.c urban_ui.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_ui_test.c
clean:
	rm -f urban_ui_test urban_test
