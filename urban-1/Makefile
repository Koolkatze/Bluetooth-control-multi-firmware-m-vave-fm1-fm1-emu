CC ?= cc
CFLAGS ?= -std=c99 -O2 -Wall -Wextra -Werror -pedantic
.PHONY: test clean
test: urban_test
	./urban_test
urban_test: urban_test.c urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_test.c
clean:
	rm -f urban_test
