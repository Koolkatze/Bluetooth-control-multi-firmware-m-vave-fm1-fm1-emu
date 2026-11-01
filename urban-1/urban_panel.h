/* URBAN-1: physical-panel event adapter, host portable.
 * Firmware HAL must map physical encoder detents/buttons to these events. */
#ifndef URBAN_PANEL_H
#define URBAN_PANEL_H
#include "urban_ui.h"
typedef enum {
    URBAN_EVT_SELECT_SONG, URBAN_EVT_SELECT_TRACKS,
    URBAN_EVT_TRACK_UP, URBAN_EVT_TRACK_DOWN,
    URBAN_EVT_NEXT_CW, URBAN_EVT_NEXT_CCW,
    URBAN_EVT_GENERATE, URBAN_EVT_PLAY_TOGGLE,
    URBAN_EVT_SEQUENCE_END
} UrbanEvent;
static void urban_panel_event(UrbanUI *ui, UrbanEvent e) {
    switch(e) {
    case URBAN_EVT_SELECT_SONG: urban_ui_page(ui,URBAN_PAGE_SONG); break;
    case URBAN_EVT_SELECT_TRACKS: urban_ui_page(ui,URBAN_PAGE_TRACKS); break;
    case URBAN_EVT_TRACK_UP:
        urban_ui_select_track(ui,(ui->selected_track+1u)%URBAN_TRACKS); break;
    case URBAN_EVT_TRACK_DOWN:
        urban_ui_select_track(ui,(ui->selected_track+URBAN_TRACKS-1u)%URBAN_TRACKS); break;
    case URBAN_EVT_NEXT_CW: urban_ui_next_knob(ui,1); break;
    case URBAN_EVT_NEXT_CCW: urban_ui_next_knob(ui,-1); break;
    case URBAN_EVT_GENERATE: urban_ui_generate(ui); break;
    case URBAN_EVT_PLAY_TOGGLE:
        ui->song.playing=(uint8_t)!ui->song.playing;
        if(!ui->song.playing) ui->song.queued=ui->song.current;
        break;
    case URBAN_EVT_SEQUENCE_END: urban_ui_on_sequence_end(ui); break;
    }
}
/* Compact 2-row display model; hardware renderer is still pending. */
typedef struct { char page[12]; char status[24]; } UrbanDisplay;
#include <stdio.h>
static void urban_panel_display(const UrbanUI *ui, UrbanDisplay *d) {
    if(ui->page==URBAN_PAGE_SONG) {
        (void)snprintf(d->page,sizeof(d->page),"SONG");
        (void)snprintf(d->status,sizeof(d->status),"NOW %c NEXT %c %s",
            'A'+ui->song.current,'A'+ui->song.queued,
            ui->song.playing?"PLAY":"STOP");
    } else {
        (void)snprintf(d->page,sizeof(d->page),"TRACKS");
        (void)snprintf(d->status,sizeof(d->status),"TRACK %u G%u S%u",
            (unsigned)ui->selected_track+1u,
            (unsigned)ui->song.tracks[ui->selected_track].genre,
            (unsigned)ui->song.tracks[ui->selected_track].subgenre);
    }
}
#endif
