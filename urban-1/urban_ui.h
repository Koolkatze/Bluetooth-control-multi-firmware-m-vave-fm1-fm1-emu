/* URBAN-1: host-testable page/controller bridge, no FM-1 hardware calls yet. */
#ifndef URBAN_UI_H
#define URBAN_UI_H
#include "urban_song.h"
typedef enum { URBAN_PAGE_SONG=0, URBAN_PAGE_TRACKS=1 } UrbanPage;
typedef struct {
    UrbanSong song;
    UrbanPage page;
    uint8_t selected_track, selected_sequence, genre, subgenre;
    uint32_t generation_serial;
} UrbanUI;
static void urban_ui_init(UrbanUI *ui, uint32_t seed) {
    memset(ui,0,sizeof(*ui));
    urban_init(&ui->song,seed);
}
static void urban_ui_page(UrbanUI *ui, UrbanPage page) { ui->page=page; }
static void urban_ui_select_track(UrbanUI *ui, unsigned index) {
    if(index<URBAN_TRACKS) ui->selected_track=(uint8_t)index;
}
static void urban_ui_set_genre(UrbanUI *ui, uint8_t genre, uint8_t subgenre) {
    if(ui->page==URBAN_PAGE_TRACKS) {
        /* Track genre must NEVER silently replace global song genre.
         * Otherwise returning HOME and pressing SAVE used the last
         * edited track's style for the whole arrangement. */
        UrbanTrack *t=&ui->song.tracks[ui->selected_track];
        t->genre=(uint8_t)(genre%URBAN_GENRES);
        t->subgenre=(uint8_t)(subgenre%4u);
    } else {
        ui->genre=(uint8_t)(genre%URBAN_GENRES);
        ui->subgenre=(uint8_t)(subgenre%4u);
        ui->song.subgenre=ui->subgenre;
        urban_song_set_genre(&ui->song,ui->genre);
    }
}
/* Generate is a NEW COMPOSITION, not merely reloading a style preset.
 * The monotonic press counter guarantees an evolving seed even in Android
 * where hardware RNG sources may return the same value; entropy adds the
 * wall/transport input time without compromising deterministic test replays.
 */
static void urban_ui_generate_entropy(UrbanUI *ui,uint32_t entropy) {
    uint32_t serial=++ui->generation_serial;
    uint32_t seed=urban_mix_seed(ui->song.seed +
         serial*0x9e3779b9u + (entropy^0xa511e9b3u));
    if(seed==ui->song.seed) seed^=0x85ebca6bu;
    if(ui->page==URBAN_PAGE_SONG) {
        urban_song_set_genre(&ui->song,ui->genre);
        /* Same genre and subgenre selected, but newly composed melody,
         * chord progression, scale/mode, bass, drum variations and timbres.
         * Preserve user key, BPM, complexity and locked tracks. */
        urban_generate_song(&ui->song,seed);
    } else {
        /* Regeneration only changes the selected track's eight phrases. */
        urban_generate_track(&ui->song,ui->selected_track,seed);
    }
}
static void urban_ui_generate(UrbanUI *ui) {
    urban_ui_generate_entropy(ui,0u);
}
static void urban_ui_song_mode(UrbanUI *ui,int automatic) {
    urban_song_set_mode(&ui->song,automatic);
}
static void urban_ui_next_knob(UrbanUI *ui,int direction) {
    if(ui->page==URBAN_PAGE_SONG) urban_queue_next(&ui->song,direction);
}
static void urban_ui_on_sequence_end(UrbanUI *ui) {
    urban_sequence_boundary(&ui->song);
}
#endif
