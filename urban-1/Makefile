CC ?= cc
CFLAGS ?= -std=c99 -O2 -Wall -Wextra -Werror -pedantic
.PHONY: test clean
test: urban_test urban_ui_test urban_panel_test urban_transport_test urban_music_test urban_runtime_test urban_rhythm_test urban_bass_test
	./urban_test
	./urban_ui_test
	./urban_panel_test
	./urban_transport_test
	./urban_music_test
	./urban_runtime_test
	./urban_rhythm_test
	./urban_bass_test
urban_test: urban_test.c urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_test.c
urban_ui_test: urban_ui_test.c urban_ui.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_ui_test.c

urban_panel_test: urban_panel_test.c urban_panel.h urban_ui.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_panel_test.c

urban_transport_test: urban_transport_test.c urban_transport.h urban_panel.h urban_ui.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_transport_test.c

urban_music_test: urban_music_test.c urban_music.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_music_test.c

urban_runtime_test: urban_runtime_test.c urban_runtime.h urban_music.h urban_transport.h urban_panel.h urban_ui.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_runtime_test.c

urban_rhythm_test: urban_rhythm_test.c urban_rhythm.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_rhythm_test.c

urban_bass_test: urban_bass_test.c urban_bass.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_bass_test.c
clean:
	rm -f urban_ui_test urban_test urban_panel_test urban_transport_test urban_music_test urban_runtime_test urban_rhythm_test urban_bass_test
