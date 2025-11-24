/*
 * Snow-Pi Map Viewer - MBTiles Offline Maps
 * Author: /x64/dumped
 * GitHub: @Ma110w
 * 
 * Reads MBTiles (SQLite) format for offline map display
 */

#include <SDL3/SDL.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TILE_SIZE 256

typedef struct {
    sqlite3 *db;
    SDL_Renderer *renderer;
    double center_lat;
    double center_lon;
    int zoom_level;
    bool active;
} MapViewer;

// Convert lat/lon to tile coordinates
void latlon_to_tile(double lat, double lon, int zoom, int *tile_x, int *tile_y) {
    double lat_rad = lat * M_PI / 180.0;
    double n = pow(2.0, zoom);
    
    *tile_x = (int)((lon + 180.0) / 360.0 * n);
    *tile_y = (int)((1.0 - log(tan(lat_rad) + 1.0 / cos(lat_rad)) / M_PI) / 2.0 * n);
}

// Initialize map viewer
bool map_viewer_init(MapViewer *viewer, const char *mbtiles_path, SDL_Renderer *renderer) {
    viewer->renderer = renderer;
    viewer->center_lat = 46.8797;  // Default to northern Ontario
    viewer->center_lon = -84.3397;
    viewer->zoom_level = 10;
    viewer->active = false;
    
    // Open MBTiles database
    int rc = sqlite3_open(mbtiles_path, &viewer->db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open MBTiles database: %s\n", sqlite3_errmsg(viewer->db));
        return false;
    }
    
    printf("MBTiles database opened successfully\n");
    return true;
}

// Get tile from database
SDL_Texture* map_viewer_get_tile(MapViewer *viewer, int zoom, int tile_x, int tile_y) {
    sqlite3_stmt *stmt;
    
    // MBTiles uses TMS (inverted Y), need to flip
    int max_y = (1 << zoom) - 1;
    int tms_y = max_y - tile_y;
    
    // Query tile from database
    const char *sql = "SELECT tile_data FROM tiles WHERE zoom_level=? AND tile_column=? AND tile_row=?";
    
    int rc = sqlite3_prepare_v2(viewer->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return NULL;
    }
    
    sqlite3_bind_int(stmt, 1, zoom);
    sqlite3_bind_int(stmt, 2, tile_x);
    sqlite3_bind_int(stmt, 3, tms_y);
    
    SDL_Texture *texture = NULL;
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        // Get tile data
        const void *blob = sqlite3_column_blob(stmt, 0);
        int blob_size = sqlite3_column_bytes(stmt, 0);
        
        // Create SDL_IOStream from memory (SDL3 API)
        SDL_IOStream *io = SDL_IOFromConstMem(blob, blob_size);
        if (io) {
            // Load image from memory (try BMP first, then PNG)
            SDL_Surface *surface = SDL_LoadBMP_IO(io, false);
            
            // Most MBTiles are PNG/JPG, try generic image loader
            if (!surface) {
                // For now, skip image loading - would need SDL3_image
                // Just create a placeholder colored tile
                surface = SDL_CreateSurface(TILE_SIZE, TILE_SIZE, SDL_PIXELFORMAT_RGB888);
                if (surface) {
                    // Fill with a pattern based on tile coords
                    SDL_FillSurfaceRect(surface, NULL, SDL_MapSurfaceRGB(surface, 100, 120, 140));
                }
            }
            
            if (surface) {
                texture = SDL_CreateTextureFromSurface(viewer->renderer, surface);
                SDL_DestroySurface(surface);
            }
            
            SDL_CloseIO(io);
        }
    }
    
    sqlite3_finalize(stmt);
    return texture;
}

// Render map view
void map_viewer_render(MapViewer *viewer, int screen_width, int screen_height) {
    if (!viewer->active) return;
    
    // Calculate center tile
    int center_tile_x, center_tile_y;
    latlon_to_tile(viewer->center_lat, viewer->center_lon, viewer->zoom_level, 
                   &center_tile_x, &center_tile_y);
    
    // Calculate how many tiles we need to cover the screen
    int tiles_x = (screen_width / TILE_SIZE) + 2;
    int tiles_y = (screen_height / TILE_SIZE) + 2;
    
    int start_tile_x = center_tile_x - tiles_x / 2;
    int start_tile_y = center_tile_y - tiles_y / 2;
    
    // Render tiles
    for (int ty = 0; ty < tiles_y; ty++) {
        for (int tx = 0; tx < tiles_x; tx++) {
            int tile_x = start_tile_x + tx;
            int tile_y = start_tile_y + ty;
            
            SDL_Texture *tile = map_viewer_get_tile(viewer, viewer->zoom_level, tile_x, tile_y);
            if (tile) {
                SDL_FRect dest = {
                    (float)(tx * TILE_SIZE - (screen_width / 2 - TILE_SIZE / 2)),
                    (float)(ty * TILE_SIZE - (screen_height / 2 - TILE_SIZE / 2)),
                    TILE_SIZE,
                    TILE_SIZE
                };
                SDL_RenderTexture(viewer->renderer, tile, NULL, &dest);
                SDL_DestroyTexture(tile);
            }
        }
    }
    
    // Draw crosshair at center (current position)
    SDL_SetRenderDrawColor(viewer->renderer, 255, 0, 0, 255);
    int cx = screen_width / 2;
    int cy = screen_height / 2;
    SDL_RenderLine(viewer->renderer, (float)(cx - 20), (float)cy, (float)(cx + 20), (float)cy);
    SDL_RenderLine(viewer->renderer, (float)cx, (float)(cy - 20), (float)cx, (float)(cy + 20));
}

// Update GPS position
void map_viewer_update_position(MapViewer *viewer, double lat, double lon) {
    viewer->center_lat = lat;
    viewer->center_lon = lon;
}

// Pan map
void map_viewer_pan(MapViewer *viewer, int dx, int dy) {
    // Convert pixel movement to lat/lon delta
    double meters_per_pixel = (40075016.686 * cos(viewer->center_lat * M_PI / 180.0)) / 
                              (pow(2.0, viewer->zoom_level + 8));
    
    viewer->center_lon += dx * meters_per_pixel / 111320.0;
    viewer->center_lat -= dy * meters_per_pixel / 110540.0;
}

// Zoom in/out
void map_viewer_zoom(MapViewer *viewer, int delta) {
    viewer->zoom_level += delta;
    if (viewer->zoom_level < 0) viewer->zoom_level = 0;
    if (viewer->zoom_level > 18) viewer->zoom_level = 18;
}

// Toggle map view
void map_viewer_toggle(MapViewer *viewer) {
    viewer->active = !viewer->active;
}

// Cleanup
void map_viewer_cleanup(MapViewer *viewer) {
    if (viewer->db) {
        sqlite3_close(viewer->db);
        viewer->db = NULL;
    }
}

