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
    ui->genre=genre; ui->subgenre=subgenre;
    if(ui->page==URBAN_PAGE_TRACKS) {
        UrbanTrack *t=&ui->song.tracks[ui->selected_track];
        t->genre=genre; t->subgenre=subgenre;
    }
}
static void urban_ui_generate(UrbanUI *ui) {
    uint32_t seed=ui->song.seed ^ (++ui->generation_serial * 0x9e3779b9u);
    if(ui->page==URBAN_PAGE_SONG) {
        urban_generate_song(&ui->song,seed);
    } else {
        urban_generate_track(&ui->song,ui->selected_track,seed);
    }
}
static void urban_ui_next_knob(UrbanUI *ui,int direction) {
    if(ui->page==URBAN_PAGE_SONG) urban_queue_next(&ui->song,direction);
}
static void urban_ui_on_sequence_end(UrbanUI *ui) {
    urban_sequence_boundary(&ui->song);
}
#endif
