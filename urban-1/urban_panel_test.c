#include "urban_panel.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void){
 UrbanUI ui; UrbanDisplay d; urban_ui_init(&ui,42);
 urban_panel_event(&ui,URBAN_EVT_GENERATE);
 urban_panel_event(&ui,URBAN_EVT_PLAY_TOGGLE);
 urban_panel_event(&ui,URBAN_EVT_NEXT_CW);
 urban_panel_display(&ui,&d);
 assert(strcmp(d.page,"SONG")==0);
 assert(strstr(d.status,"NOW A NEXT B")!=NULL);
 urban_panel_event(&ui,URBAN_EVT_SEQUENCE_END);
 urban_panel_display(&ui,&d);
 assert(strstr(d.status,"NOW B NEXT B")!=NULL);
 urban_panel_event(&ui,URBAN_EVT_SELECT_TRACKS);
 urban_panel_event(&ui,URBAN_EVT_TRACK_UP);
 urban_panel_display(&ui,&d);
 assert(strcmp(d.page,"TRACKS")==0 && ui.selected_track==1);
 puts("URBAN-1 panel adapter tests passed");
 return 0;
}
