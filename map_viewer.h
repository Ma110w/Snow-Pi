/*
 * Snow-Pi Map Viewer Header
 * Author: /x64/dumped
 */

#ifndef MAP_VIEWER_H
#define MAP_VIEWER_H

#include <SDL3/SDL.h>
#include <sqlite3.h>
#include <stdbool.h>

typedef struct {
    sqlite3 *db;
    SDL_Renderer *renderer;
    double center_lat;
    double center_lon;
    int zoom_level;
    bool active;
} MapViewer;

bool map_viewer_init(MapViewer *viewer, const char *mbtiles_path, SDL_Renderer *renderer);
void map_viewer_render(MapViewer *viewer, int screen_width, int screen_height);
void map_viewer_update_position(MapViewer *viewer, double lat, double lon);
void map_viewer_pan(MapViewer *viewer, int dx, int dy);
void map_viewer_zoom(MapViewer *viewer, int delta);
void map_viewer_toggle(MapViewer *viewer);
void map_viewer_cleanup(MapViewer *viewer);

#endif

