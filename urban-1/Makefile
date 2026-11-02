CC ?= cc
CFLAGS ?= -std=c99 -O2 -Wall -Wextra -Werror -Wno-unused-function -pedantic
.PHONY: test clean
test: urban_test urban_ui_test urban_panel_test urban_transport_test urban_music_test urban_runtime_test urban_rhythm_test urban_bass_test urban_audio_test urban_arrangement_test urban_style_test urban_composer_test urban_score_test urban_lead_mix_test urban_genre_fx_test urban_subgenre_test urban_drum_sync_test urban_hiphop_test urban_sfx_test urban_trance_test urban_scales_test urban_groove_v2_test urban_jazz_test
	./urban_test
	./urban_ui_test
	./urban_panel_test
	./urban_transport_test
	./urban_music_test
	./urban_runtime_test
	./urban_rhythm_test
	./urban_bass_test
	./urban_audio_test
	./urban_arrangement_test
	./urban_style_test
	./urban_composer_test
	./urban_score_test
	./urban_lead_mix_test
	./urban_genre_fx_test
	./urban_subgenre_test
	./urban_drum_sync_test
	./urban_hiphop_test
	./urban_sfx_test
	./urban_trance_test
	./urban_scales_test
	./urban_groove_v2_test
	./urban_jazz_test
urban_test: urban_test.c urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_test.c
urban_ui_test: urban_ui_test.c urban_ui.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_ui_test.c

urban_panel_test: urban_panel_test.c urban_panel.h urban_ui.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_panel_test.c

urban_transport_test: urban_transport_test.c urban_transport.h urban_panel.h urban_ui.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_transport_test.c

urban_music_test: urban_music_test.c urban_music.h urban_rhythm.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_music_test.c

urban_runtime_test: urban_runtime_test.c urban_runtime.h urban_bass.h urban_rhythm.h urban_music.h urban_transport.h urban_panel.h urban_ui.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_runtime_test.c

urban_rhythm_test: urban_rhythm_test.c urban_rhythm.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_rhythm_test.c

urban_bass_test: urban_bass_test.c urban_bass.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_bass_test.c

urban_audio_test: urban_audio_test.c urban_audio.h urban_runtime.h urban_bass.h urban_music.h urban_transport.h urban_panel.h urban_ui.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_audio_test.c

urban_arrangement_test: urban_arrangement_test.c urban_runtime.h urban_music.h urban_rhythm.h urban_bass.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_arrangement_test.c

urban_style_test: urban_style_test.c urban_style.h urban_rhythm.h urban_bass.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_style_test.c

urban_composer_test: urban_composer_test.c urban_runtime.h urban_harmony.h urban_music.h urban_rhythm.h urban_bass.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_composer_test.c

urban_score_test: urban_score_test.c urban_chord_score.h urban_instruments.h urban_runtime.h urban_harmony.h urban_music.h urban_rhythm.h urban_bass.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_score_test.c

urban_lead_mix_test: urban_lead_mix_test.c urban_mix.h urban_lead.h urban_runtime.h urban_music.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_lead_mix_test.c

urban_genre_fx_test: urban_genre_fx_test.c urban_fx.h urban_lead.h urban_mix.h urban_runtime.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_genre_fx_test.c
urban_subgenre_test: urban_subgenre_test.c urban_bass.h urban_rhythm.h urban_chord_score.h urban_instruments.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_subgenre_test.c

urban_drum_sync_test: urban_drum_sync_test.c urban_drum_sync.h
	$(CC) $(CFLAGS) -o $@ urban_drum_sync_test.c

urban_hiphop_test: urban_hiphop_test.c urban_fx.h urban_chord_score.h urban_runtime.h urban_instruments.h urban_rhythm.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_hiphop_test.c
urban_sfx_test: urban_sfx_test.c urban_sfx.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_sfx_test.c

urban_trance_test: urban_trance_test.c urban_lead.h urban_music.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_trance_test.c

urban_scales_test: urban_scales_test.c urban_scales.h urban_song.h urban_harmony.h urban_bass.h urban_music.h urban_ui.h
	$(CC) $(CFLAGS) -o $@ urban_scales_test.c

urban_groove_v2_test: urban_groove_v2_test.c urban_bass.h urban_chord_score.h urban_rhythm.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_groove_v2_test.c

urban_jazz_test: urban_jazz_test.c urban_instruments.h urban_song.h
	$(CC) $(CFLAGS) -o $@ urban_jazz_test.c

clean:
	rm -f urban_ui_test urban_test urban_panel_test urban_transport_test urban_music_test urban_runtime_test urban_rhythm_test urban_bass_test urban_audio_test urban_arrangement_test urban_style_test urban_composer_test urban_score_test urban_lead_mix_test urban_genre_fx_test urban_subgenre_test urban_drum_sync_test urban_hiphop_test urban_sfx_test urban_trance_test urban_scales_test urban_groove_v2_test urban_jazz_test
