/*
 * Snow-Pi Digital Dashboard
 * Author: /x64/dumped
 * GitHub: @Ma110w
 * 
 * SDL3-based HUD for snowmobile digital dash
 * Optimized for Rock64 and Raspberry Pi Zero 2W
 */

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "map_viewer.h"

#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep((x) * 1000)
#else
#include <unistd.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 480
#define FPS 30
#define FRAME_DELAY (1000 / FPS)

// Colors
typedef struct {
    Uint8 r, g, b, a;
} Color;

// Polaris color scheme
static const Color COLOR_BG = {10, 14, 39, 255};
static const Color COLOR_PRIMARY = {0, 212, 255, 255};        // Cyan for normal displays
static const Color COLOR_POLARIS_AMBER = {255, 180, 0, 255}; // Polaris amber warning
static const Color COLOR_POLARIS_RED = {255, 0, 0, 255};     // Polaris red critical
static const Color COLOR_SUCCESS = {0, 255, 136, 255};       // Green for good
static const Color COLOR_GLASS = {255, 255, 255, 25};
static const Color COLOR_BORDER = {255, 255, 255, 60};
static const Color COLOR_GAUGE_BG = {60, 80, 120, 80};
// Aliases for compatibility
static const Color COLOR_WARNING = {255, 0, 0, 255};         // Same as POLARIS_RED
static const Color COLOR_ACCENT = {255, 180, 0, 255};        // Same as POLARIS_AMBER

// Drive modes
typedef enum {
    MODE_DRIVE,
    MODE_REVERSE
} DriveMode;

// Display modes (Polaris-style scrolling)
typedef enum {
    DISPLAY_ODOMETER,
    DISPLAY_TRIP_A,
    DISPLAY_TRIP_B,
    DISPLAY_ENGINE_HOURS
} DisplayMode;

// Dashboard data
typedef struct {
    float speed;
    float rpm;
    float target_rpm;  // Throttle target
    float throttle;    // 0.0 to 1.0
    float engine_temp;
    float coolant_temp;
    float belt_temp;   // Critical for Polaris 600
    float fuel_level;
    float voltage;
    float odometer;
    float trip_a;
    float trip_b;
    float engine_hours;
    double latitude;
    double longitude;
    DriveMode drive_mode;
    DisplayMode display_mode;
    bool warning_engine_temp;
    bool warning_coolant_temp;
    bool warning_belt_temp;
    bool warning_low_fuel;
    bool warning_low_voltage;
} DashboardData;

// Application context
typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font_digital_large;
    TTF_Font *font_digital_medium;
    TTF_Font *font_digital_small;
    TTF_Font *font_arial_bold;
    TTF_Font *font_arial_small;
    DashboardData data;
    MapViewer map_viewer;
    bool running;
    bool boot_complete;
    bool show_map;
    Uint32 last_frame_time;
    Uint32 boot_start_time;
} AppContext;

// Function prototypes
void init_sdl(AppContext *ctx);
void cleanup_sdl(AppContext *ctx);
void handle_events(AppContext *ctx);
void update_dashboard(AppContext *ctx);
void render_dashboard(AppContext *ctx);
void draw_filled_circle(SDL_Renderer *renderer, int cx, int cy, int radius);
void draw_circle(SDL_Renderer *renderer, int cx, int cy, int radius);
void draw_arc(SDL_Renderer *renderer, int cx, int cy, int radius, float start_angle, float end_angle, int thickness);
void draw_rounded_rect(SDL_Renderer *renderer, int x, int y, int w, int h, int radius);
void draw_filled_rounded_rect(SDL_Renderer *renderer, int x, int y, int w, int h, int radius);
void draw_gauge(AppContext *ctx, int cx, int cy, int radius, float value, float max_value, bool is_primary);
void draw_number(SDL_Renderer *renderer, int value, int x, int y, int size, Color color);
void draw_digit(SDL_Renderer *renderer, int digit, int x, int y, int width, int height, Color color);
void draw_label(SDL_Renderer *renderer, const char *text, int x, int y, int size, Color color);
void draw_text_ttf(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, Color color, bool centered);
void draw_drive_mode(AppContext *ctx, int x, int y, int size);
void draw_boot_screen(AppContext *ctx);

int main(int argc, char *argv[]) {
    (void)argc;  // Unused
    (void)argv;  // Unused
    
    AppContext ctx = {0};
    
    printf("=======================================================\n");
    printf("Snow-Pi Digital Dashboard\n");
    printf("Author: /x64/dumped | GitHub: @Ma110w\n");
    printf("=======================================================\n\n");
    
    init_sdl(&ctx);
    
    ctx.running = true;
    ctx.boot_complete = false;
    ctx.show_map = false;
    ctx.boot_start_time = SDL_GetTicks();
    ctx.last_frame_time = SDL_GetTicks();
    ctx.data.drive_mode = MODE_DRIVE;
    ctx.data.display_mode = DISPLAY_ODOMETER;
    ctx.data.throttle = 0.0f;
    ctx.data.target_rpm = 0.0f;
    
    // Initialize map viewer
    if (!map_viewer_init(&ctx.map_viewer, "osm-2020-02-10-v3.11_canada_ontario.mbtiles", ctx.renderer)) {
        printf("Warning: Could not load map tiles. Map view disabled.\n");
    }
    
    // Main loop
    while (ctx.running) {
        Uint32 frame_start = SDL_GetTicks();
        
        handle_events(&ctx);
        update_dashboard(&ctx);
        render_dashboard(&ctx);
        
        // Frame rate limiting
        Uint32 frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < FRAME_DELAY) {
            SDL_Delay(FRAME_DELAY - frame_time);
        }
    }
    
    cleanup_sdl(&ctx);
    return 0;
}

void init_sdl(AppContext *ctx) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        exit(1);
    }
    
    // Create window with renderer (SDL3 style)
    if (!SDL_CreateWindowAndRenderer("Snow-Pi Dashboard", WINDOW_WIDTH, WINDOW_HEIGHT, 
                                      0, &ctx->window, &ctx->renderer)) {
        fprintf(stderr, "Window/Renderer creation failed: %s\n", SDL_GetError());
        exit(1);
    }
    
    // Enable VSync
    SDL_SetRenderVSync(ctx->renderer, 1);
    
    SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND);
    
    // Initialize TTF
    if (!TTF_Init()) {
        fprintf(stderr, "TTF init failed: %s\n", SDL_GetError());
        exit(1);
    }
    
    // Load fonts
    ctx->font_digital_large = TTF_OpenFont("digital.ttf", 64);
    ctx->font_digital_medium = TTF_OpenFont("digital.ttf", 42);
    ctx->font_digital_small = TTF_OpenFont("digital.ttf", 28);
    ctx->font_arial_bold = TTF_OpenFont("Arial.ttf", 24);
    ctx->font_arial_small = TTF_OpenFont("Arial.ttf", 16);
    
    if (!ctx->font_digital_large || !ctx->font_digital_medium || !ctx->font_digital_small ||
        !ctx->font_arial_bold || !ctx->font_arial_small) {
        fprintf(stderr, "Font loading failed: %s\n", SDL_GetError());
        fprintf(stderr, "Make sure digital.ttf and Arial.ttf are in the same directory\n");
        exit(1);
    }
    
    printf("SDL3 and fonts initialized successfully\n");
}

void cleanup_sdl(AppContext *ctx) {
    map_viewer_cleanup(&ctx->map_viewer);
    if (ctx->font_digital_large) TTF_CloseFont(ctx->font_digital_large);
    if (ctx->font_digital_medium) TTF_CloseFont(ctx->font_digital_medium);
    if (ctx->font_digital_small) TTF_CloseFont(ctx->font_digital_small);
    if (ctx->font_arial_bold) TTF_CloseFont(ctx->font_arial_bold);
    if (ctx->font_arial_small) TTF_CloseFont(ctx->font_arial_small);
    if (ctx->renderer) SDL_DestroyRenderer(ctx->renderer);
    if (ctx->window) SDL_DestroyWindow(ctx->window);
    TTF_Quit();
    SDL_Quit();
}

void handle_events(AppContext *ctx) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                ctx->running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE || event.key.key == SDLK_Q) {
                    ctx->running = false;
                }
                // M to toggle between Drive/Reverse
                else if (event.key.key == SDLK_M) {
                    ctx->data.drive_mode = (ctx->data.drive_mode == MODE_DRIVE) ? MODE_REVERSE : MODE_DRIVE;
                }
                // S to scroll display modes (Polaris-style)
                else if (event.key.key == SDLK_S) {
                    ctx->data.display_mode = (ctx->data.display_mode + 1) % 4;
                }
                // TAB to toggle map view
                else if (event.key.key == SDLK_TAB) {
                    ctx->show_map = !ctx->show_map;
                    map_viewer_toggle(&ctx->map_viewer);
                }
                // Arrow keys for map panning (when map is shown)
                else if (ctx->show_map) {
                    if (event.key.key == SDLK_LEFT) map_viewer_pan(&ctx->map_viewer, -50, 0);
                    else if (event.key.key == SDLK_RIGHT) map_viewer_pan(&ctx->map_viewer, 50, 0);
                    else if (event.key.key == SDLK_UP && !keys[SDL_SCANCODE_R]) map_viewer_pan(&ctx->map_viewer, 0, -50);
                    else if (event.key.key == SDLK_DOWN) map_viewer_pan(&ctx->map_viewer, 0, 50);
                    else if (event.key.key == SDLK_EQUALS || event.key.key == SDLK_PLUS) map_viewer_zoom(&ctx->map_viewer, 1);
                    else if (event.key.key == SDLK_MINUS) map_viewer_zoom(&ctx->map_viewer, -1);
                }
                else if (event.key.key == SDLK_SPACE) {
                    // Space to skip boot screen
                    ctx->boot_complete = true;
                }
                break;
        }
    }
    
    // Throttle control - hold key to throttle
    const bool *keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_R] || keys[SDL_SCANCODE_UP]) {
        // Throttle up
        ctx->data.throttle = fminf(ctx->data.throttle + 0.05f, 1.0f);
    } else {
        // Release throttle
        ctx->data.throttle = fmaxf(ctx->data.throttle - 0.08f, 0.0f);
    }
}

void update_dashboard(AppContext *ctx) {
    // Check if boot sequence is complete
    if (!ctx->boot_complete) {
        Uint32 elapsed = SDL_GetTicks() - ctx->boot_start_time;
        if (elapsed > 3000) {  // 3 second boot
            ctx->boot_complete = true;
            // Initialize data to zero on boot complete
            ctx->data.speed = 0;
            ctx->data.rpm = 1000;  // Idle RPM
            ctx->data.target_rpm = 1000;
            ctx->data.engine_temp = 70;
            ctx->data.coolant_temp = 65;
            ctx->data.belt_temp = 80;  // Belt starts cool
            ctx->data.fuel_level = 85;
            ctx->data.voltage = 13.8f;
            ctx->data.odometer = 1234.5f;
            ctx->data.trip_a = 0;
            ctx->data.trip_b = 0;
            ctx->data.engine_hours = 127.5f;  // Example hours
        }
        return;
    }
    
    // Calculate delta time
    Uint32 current_time = SDL_GetTicks();
    float dt = (current_time - ctx->last_frame_time) / 1000.0f; // Delta in seconds
    ctx->last_frame_time = current_time;
    
    // Realistic engine physics
    float idle_rpm = 1000.0f;
    float max_rpm = 9000.0f;
    
    // Target RPM based on throttle
    ctx->data.target_rpm = idle_rpm + (max_rpm - idle_rpm) * ctx->data.throttle;
    
    // RPM responds to throttle with some lag (acceleration/deceleration)
    float rpm_diff = ctx->data.target_rpm - ctx->data.rpm;
    float rpm_accel_rate = 3000.0f; // RPM per second when throttling
    float rpm_decel_rate = 2000.0f; // RPM per second when releasing
    
    if (rpm_diff > 0) {
        ctx->data.rpm += fminf(rpm_diff, rpm_accel_rate * dt);
    } else {
        ctx->data.rpm += fmaxf(rpm_diff, -rpm_decel_rate * dt);
    }
    
    // Speed is derived from RPM and gear (simplified)
    // In forward mode, higher RPM = higher speed
    // In reverse, speed is limited
    float target_speed = 0;
    
    if (ctx->data.drive_mode == MODE_DRIVE) {
        // Forward: speed proportional to RPM above idle
        float rpm_above_idle = fmaxf(0, ctx->data.rpm - idle_rpm);
        target_speed = (rpm_above_idle / (max_rpm - idle_rpm)) * 120.0f; // Max 120 MPH
    } else {
        // Reverse: limited speed
        float rpm_above_idle = fmaxf(0, ctx->data.rpm - idle_rpm);
        target_speed = -(rpm_above_idle / (max_rpm - idle_rpm)) * 25.0f; // Max 25 MPH reverse
    }
    
    // Speed has momentum and drag
    float speed_diff = target_speed - ctx->data.speed;
    float accel_rate = 40.0f; // MPH per second
    float drag_rate = 60.0f;  // Deceleration from drag
    
    if (fabsf(speed_diff) < 0.1f) {
        ctx->data.speed = target_speed;
    } else if (speed_diff > 0) {
        ctx->data.speed += fminf(speed_diff, accel_rate * dt);
    } else {
        ctx->data.speed += fmaxf(speed_diff, -drag_rate * dt);
    }
    
    // Engine temp increases with RPM and throttle
    float temp_increase = ctx->data.throttle * 0.5f * dt;
    float temp_cooling = 1.0f * dt;
    ctx->data.engine_temp += temp_increase - temp_cooling;
    ctx->data.engine_temp = fmaxf(70.0f, fminf(ctx->data.engine_temp, 250.0f));
    
    // Coolant temp follows engine temp
    float coolant_diff = ctx->data.engine_temp - ctx->data.coolant_temp;
    ctx->data.coolant_temp += coolant_diff * 0.1f * dt;
    
    // Belt temp - CRITICAL for Polaris 600!
    // Belt heats up faster than engine with high RPM and speed mismatch
    float belt_heating = ctx->data.throttle * 1.2f * dt;  // Heats faster than engine
    float belt_cooling = (ctx->data.speed / 120.0f) * 2.0f * dt;  // Airflow cooling
    ctx->data.belt_temp += belt_heating - belt_cooling;
    ctx->data.belt_temp = fmaxf(80.0f, fminf(ctx->data.belt_temp, 220.0f));
    
    // Fuel consumption based on throttle
    if (ctx->data.throttle > 0.1f) {
        ctx->data.fuel_level -= ctx->data.throttle * 0.1f * dt;
        ctx->data.fuel_level = fmaxf(0, ctx->data.fuel_level);
    }
    
    // Update odometer, trips, and engine hours
    float distance = fabsf(ctx->data.speed) * dt / 3600.0f; // Convert MPH to miles
    ctx->data.odometer += distance;
    ctx->data.trip_a += distance;
    ctx->data.trip_b += distance;
    ctx->data.engine_hours += dt / 3600.0f;  // Convert seconds to hours
    
    // Voltage fluctuates slightly with RPM
    float voltage_base = 13.8f;
    ctx->data.voltage = voltage_base + (ctx->data.rpm / max_rpm) * 0.3f + ((rand() % 10) - 5) / 100.0f;
    
    // Update warnings based on current values (Polaris thresholds)
    ctx->data.warning_engine_temp = ctx->data.engine_temp > 220.0f;
    ctx->data.warning_coolant_temp = ctx->data.coolant_temp > 210.0f;
    ctx->data.warning_belt_temp = ctx->data.belt_temp > 180.0f;  // Critical for belt life!
    ctx->data.warning_low_fuel = ctx->data.fuel_level < 20.0f;
    ctx->data.warning_low_voltage = ctx->data.voltage < 12.5f;
}

void simulate_sensor_data(DashboardData *data) {
    static double time_offset = 0.0;
    time_offset += 0.03;
    
    // Realistic snowmobile data simulation
    data->speed = fabs(40.0f + 30.0f * sin(time_offset / 3.0)) + ((rand() % 40) - 20) / 10.0f;
    data->rpm = data->speed * 100.0f + ((rand() % 100) - 50);
    data->engine_temp = 150.0f + 20.0f * sin(time_offset / 20.0) + ((rand() % 20) - 10) / 10.0f;
    data->coolant_temp = data->engine_temp - 10.0f + ((rand() % 40) - 20) / 10.0f;
    data->fuel_level = fmax(10.0f, 85.0f - time_offset * 0.5f);
    if (data->fuel_level < 15.0f) time_offset = 0.0; // Reset for demo
    data->voltage = 13.8f + ((rand() % 40) - 20) / 100.0f;
    data->odometer = 1234.5f + time_offset;
    data->trip_a = fmod(time_offset, 100.0);
    data->trip_b = fmod(time_offset, 100.0);
    data->latitude = 46.8797 + ((rand() % 20) - 10) / 10000.0;
    data->longitude = -113.9964 + ((rand() % 20) - 10) / 10000.0;
    
    // Warning flags
    data->warning_engine_temp = data->engine_temp > 220.0f;
    data->warning_coolant_temp = data->coolant_temp > 210.0f;
    data->warning_low_fuel = data->fuel_level < 20.0f;
    data->warning_low_voltage = data->voltage < 12.5f;
}

void render_dashboard(AppContext *ctx) {
    // Clear with background gradient (simplified to solid color for performance)
    SDL_SetRenderDrawColor(ctx->renderer, COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, COLOR_BG.a);
    SDL_RenderClear(ctx->renderer);
    
    // Show boot screen if not complete
    if (!ctx->boot_complete) {
        draw_boot_screen(ctx);
        SDL_RenderPresent(ctx->renderer);
        return;
    }
    
    // Show map view if toggled
    if (ctx->show_map) {
        map_viewer_render(&ctx->map_viewer, WINDOW_WIDTH, WINDOW_HEIGHT);
        
        // Draw minimal overlay with key info
        SDL_SetRenderDrawColor(ctx->renderer, 10, 10, 10, 200);
        SDL_FRect overlay = {10, 10, 250, 80};
        SDL_RenderFillRect(ctx->renderer, &overlay);
        
        char info[128];
        snprintf(info, sizeof(info), "%.1f KM/H", fabsf(ctx->data.speed) * 1.60934f);
        draw_text_ttf(ctx->renderer, ctx->font_digital_medium, info, 20, 20, COLOR_PRIMARY, false);
        
        draw_text_ttf(ctx->renderer, ctx->font_arial_small, "TAB: Dashboard", 20, 60, COLOR_PRIMARY, false);
        
        SDL_RenderPresent(ctx->renderer);
        return;
    }
    
    // Draw header bar
    SDL_SetRenderDrawColor(ctx->renderer, COLOR_GLASS.r, COLOR_GLASS.g, COLOR_GLASS.b, COLOR_GLASS.a);
    draw_filled_rounded_rect(ctx->renderer, 10, 10, WINDOW_WIDTH - 20, 50, 10);
    SDL_SetRenderDrawColor(ctx->renderer, COLOR_BORDER.r, COLOR_BORDER.g, COLOR_BORDER.b, COLOR_BORDER.a);
    draw_rounded_rect(ctx->renderer, 10, 10, WINDOW_WIDTH - 20, 50, 10);
    
    // Logo (Polaris branding)
    draw_text_ttf(ctx->renderer, ctx->font_arial_bold, "POLARIS", 25, 15, COLOR_PRIMARY, false);
    
    // Clock (Polaris feature - top right)
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char clock_str[16];
    snprintf(clock_str, sizeof(clock_str), "%02d:%02d", t->tm_hour, t->tm_min);
    draw_text_ttf(ctx->renderer, ctx->font_digital_small, clock_str, WINDOW_WIDTH - 100, 25, COLOR_PRIMARY, false);
    
    // Connection indicator (green dot)
    SDL_SetRenderDrawColor(ctx->renderer, COLOR_SUCCESS.r, COLOR_SUCCESS.g, COLOR_SUCCESS.b, 255);
    draw_filled_circle(ctx->renderer, WINDOW_WIDTH - 30, 35, 6);
    
    // Drive mode indicator (large, top center)
    draw_drive_mode(ctx, WINDOW_WIDTH / 2, 35, 40);
    
    // Main gauges (moved down to not overlap header)
    int gauge_y = 200;
    int speed_x = WINDOW_WIDTH / 2 - 150;
    int rpm_x = WINDOW_WIDTH / 2 + 150;
    
    // Speed gauge (large, left)
    draw_gauge(ctx, speed_x, gauge_y, 110, ctx->data.speed, 120.0f, true);
    
    // Speed number (digital font) - show absolute value for display, convert to KM/H
    char speed_str[16];
    int speed_kmh = (int)(fabsf(ctx->data.speed) * 1.60934f);  // Convert MPH to KM/H
    snprintf(speed_str, sizeof(speed_str), "%d", speed_kmh);
    draw_text_ttf(ctx->renderer, ctx->font_digital_large, speed_str, speed_x, gauge_y - 10, COLOR_PRIMARY, true);
    draw_text_ttf(ctx->renderer, ctx->font_arial_small, "KM/H", speed_x, gauge_y + 50, COLOR_PRIMARY, true);
    
    // RPM gauge (right)
    draw_gauge(ctx, rpm_x, gauge_y, 85, ctx->data.rpm, 9000.0f, false);
    
    // RPM number (digital font) - show actual RPM
    char rpm_str[16];
    snprintf(rpm_str, sizeof(rpm_str), "%d", (int)ctx->data.rpm);
    draw_text_ttf(ctx->renderer, ctx->font_digital_medium, rpm_str, rpm_x, gauge_y - 5, COLOR_PRIMARY, true);
    draw_text_ttf(ctx->renderer, ctx->font_arial_small, "RPM", rpm_x, gauge_y + 35, COLOR_PRIMARY, true);
    
    // Info panels at bottom
    int panel_y = 350;
    int panel_w = 180;
    int panel_h = 110;
    int panel_spacing = 10;
    int start_x = (WINDOW_WIDTH - (panel_w * 4 + panel_spacing * 3)) / 2;
    
    // Temperature panel
    SDL_SetRenderDrawColor(ctx->renderer, COLOR_GLASS.r, COLOR_GLASS.g, COLOR_GLASS.b, COLOR_GLASS.a);
    draw_filled_rounded_rect(ctx->renderer, start_x, panel_y, panel_w, panel_h, 10);
    SDL_SetRenderDrawColor(ctx->renderer, COLOR_BORDER.r, COLOR_BORDER.g, COLOR_BORDER.b, COLOR_BORDER.a);
    draw_rounded_rect(ctx->renderer, start_x, panel_y, panel_w, panel_h, 10);
    
    draw_text_ttf(ctx->renderer, ctx->font_arial_bold, "TEMP", start_x + panel_w/2, panel_y + 12, COLOR_PRIMARY, true);
    
    // Engine temp (Polaris amber/red scheme)
    Color temp_color = ctx->data.warning_engine_temp ? COLOR_POLARIS_RED : COLOR_PRIMARY;
    char eng_temp_str[16];
    snprintf(eng_temp_str, sizeof(eng_temp_str), "%d", (int)ctx->data.engine_temp);
    draw_text_ttf(ctx->renderer, ctx->font_digital_small, eng_temp_str, start_x + 20, panel_y + 40, temp_color, false);
    draw_text_ttf(ctx->renderer, ctx->font_arial_small, "ENG", start_x + 20, panel_y + 75, temp_color, false);
    
    // Belt temp - CRITICAL!
    temp_color = ctx->data.warning_belt_temp ? COLOR_POLARIS_RED : 
                 (ctx->data.belt_temp > 160.0f ? COLOR_POLARIS_AMBER : COLOR_PRIMARY);
    char belt_temp_str[16];
    snprintf(belt_temp_str, sizeof(belt_temp_str), "%d", (int)ctx->data.belt_temp);
    draw_text_ttf(ctx->renderer, ctx->font_digital_small, belt_temp_str, start_x + 100, panel_y + 40, temp_color, false);
    draw_text_ttf(ctx->renderer, ctx->font_arial_small, "BELT", start_x + 100, panel_y + 75, temp_color, false);
    
    // Fuel panel
    start_x += panel_w + panel_spacing;
    SDL_SetRenderDrawColor(ctx->renderer, COLOR_GLASS.r, COLOR_GLASS.g, COLOR_GLASS.b, COLOR_GLASS.a);
    draw_filled_rounded_rect(ctx->renderer, start_x, panel_y, panel_w, panel_h, 10);
    SDL_SetRenderDrawColor(ctx->renderer, COLOR_BORDER.r, COLOR_BORDER.g, COLOR_BORDER.b, COLOR_BORDER.a);
    draw_rounded_rect(ctx->renderer, start_x, panel_y, panel_w, panel_h, 10);
    
    draw_text_ttf(ctx->renderer, ctx->font_arial_bold, "FUEL", start_x + panel_w/2, panel_y + 12, COLOR_PRIMARY, true);
    
    // Fuel bar
    int bar_x = start_x + 10;
    int bar_y = panel_y + 35;
    int bar_w = panel_w - 20;
    int bar_h = 15;
    
    SDL_SetRenderDrawColor(ctx->renderer, 40, 40, 40, 255);
    SDL_FRect bar_bg = {(float)bar_x, (float)bar_y, (float)bar_w, (float)bar_h};
    SDL_RenderFillRect(ctx->renderer, &bar_bg);
    
    Color fuel_color = ctx->data.warning_low_fuel ? COLOR_WARNING : COLOR_SUCCESS;
    SDL_SetRenderDrawColor(ctx->renderer, fuel_color.r, fuel_color.g, fuel_color.b, 255);
    SDL_FRect bar_fill = {(float)bar_x, (float)bar_y, bar_w * ctx->data.fuel_level / 100.0f, (float)bar_h};
    SDL_RenderFillRect(ctx->renderer, &bar_fill);
    
    // Fuel percentage number (below bar, not overlapping)
    char fuel_str[16];
    snprintf(fuel_str, sizeof(fuel_str), "%d%%", (int)ctx->data.fuel_level);
    draw_text_ttf(ctx->renderer, ctx->font_digital_medium, fuel_str, start_x + panel_w/2, panel_y + 70, fuel_color, true);
    
    // Trip info panel
    start_x += panel_w + panel_spacing;
    SDL_SetRenderDrawColor(ctx->renderer, COLOR_GLASS.r, COLOR_GLASS.g, COLOR_GLASS.b, COLOR_GLASS.a);
    draw_filled_rounded_rect(ctx->renderer, start_x, panel_y, panel_w, panel_h, 10);
    SDL_SetRenderDrawColor(ctx->renderer, COLOR_BORDER.r, COLOR_BORDER.g, COLOR_BORDER.b, COLOR_BORDER.a);
    draw_rounded_rect(ctx->renderer, start_x, panel_y, panel_w, panel_h, 10);
    
    // Scrolling display mode (Polaris-style)
    const char *mode_label;
    char display_value[32];
    
    switch (ctx->data.display_mode) {
        case DISPLAY_ODOMETER:
            mode_label = "ODO";
            snprintf(display_value, sizeof(display_value), "%.1f", ctx->data.odometer);
            break;
        case DISPLAY_TRIP_A:
            mode_label = "TRIP A";
            snprintf(display_value, sizeof(display_value), "%.1f", ctx->data.trip_a);
            break;
        case DISPLAY_TRIP_B:
            mode_label = "TRIP B";
            snprintf(display_value, sizeof(display_value), "%.1f", ctx->data.trip_b);
            break;
        case DISPLAY_ENGINE_HOURS:
            mode_label = "HRS";
            snprintf(display_value, sizeof(display_value), "%.1f", ctx->data.engine_hours);
            break;
        default:
            mode_label = "ODO";
            snprintf(display_value, sizeof(display_value), "%.1f", ctx->data.odometer);
    }
    
    draw_text_ttf(ctx->renderer, ctx->font_arial_bold, mode_label, start_x + panel_w/2, panel_y + 12, COLOR_PRIMARY, true);
    draw_text_ttf(ctx->renderer, ctx->font_digital_medium, display_value, start_x + panel_w/2, panel_y + 55, COLOR_PRIMARY, true);
    
    // System panel
    start_x += panel_w + panel_spacing;
    SDL_SetRenderDrawColor(ctx->renderer, COLOR_GLASS.r, COLOR_GLASS.g, COLOR_GLASS.b, COLOR_GLASS.a);
    draw_filled_rounded_rect(ctx->renderer, start_x, panel_y, panel_w, panel_h, 10);
    SDL_SetRenderDrawColor(ctx->renderer, COLOR_BORDER.r, COLOR_BORDER.g, COLOR_BORDER.b, COLOR_BORDER.a);
    draw_rounded_rect(ctx->renderer, start_x, panel_y, panel_w, panel_h, 10);
    
    draw_text_ttf(ctx->renderer, ctx->font_arial_bold, "SYSTEM", start_x + panel_w/2, panel_y + 12, COLOR_PRIMARY, true);
    
    // Battery voltage with decimal
    Color volt_color = ctx->data.warning_low_voltage ? COLOR_WARNING : COLOR_SUCCESS;
    char volt_str[16];
    snprintf(volt_str, sizeof(volt_str), "%.1fV", ctx->data.voltage);
    draw_text_ttf(ctx->renderer, ctx->font_digital_medium, volt_str, start_x + panel_w/2, panel_y + 55, volt_color, true);
    
    // Warning overlay (Polaris-style critical warnings)
    bool has_warnings = ctx->data.warning_engine_temp || ctx->data.warning_belt_temp || 
                       ctx->data.warning_low_fuel || ctx->data.warning_low_voltage;
    
    if (has_warnings) {
        // Semi-transparent overlay
        SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 200);
        SDL_FRect overlay = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
        SDL_RenderFillRect(ctx->renderer, &overlay);
        
        // Warning box
        int warn_w = 500;
        int warn_h = 200;
        int warn_x = (WINDOW_WIDTH - warn_w) / 2;
        int warn_y = (WINDOW_HEIGHT - warn_h) / 2;
        
        SDL_SetRenderDrawColor(ctx->renderer, 40, 10, 10, 230);
        draw_filled_rounded_rect(ctx->renderer, warn_x, warn_y, warn_w, warn_h, 15);
        SDL_SetRenderDrawColor(ctx->renderer, COLOR_WARNING.r, COLOR_WARNING.g, COLOR_WARNING.b, 255);
        draw_rounded_rect(ctx->renderer, warn_x, warn_y, warn_w, warn_h, 15);
        draw_rounded_rect(ctx->renderer, warn_x + 2, warn_y + 2, warn_w - 4, warn_h - 4, 13);
        
        // Warning triangle (Polaris red)
        SDL_SetRenderDrawColor(ctx->renderer, COLOR_POLARIS_RED.r, COLOR_POLARIS_RED.g, COLOR_POLARIS_RED.b, 255);
        for (int i = 0; i < 5; i++) {
            SDL_RenderLine(ctx->renderer, warn_x + warn_w/2 - 40 + i, warn_y + 80, warn_x + warn_w/2, warn_y + 40 - i);
            SDL_RenderLine(ctx->renderer, warn_x + warn_w/2, warn_y + 40 - i, warn_x + warn_w/2 + 40 - i, warn_y + 80);
            SDL_RenderLine(ctx->renderer, warn_x + warn_w/2 - 40 + i, warn_y + 80, warn_x + warn_w/2 + 40 - i, warn_y + 80);
        }
        
        // Warning messages (Polaris-style)
        int msg_y = warn_y + 110;
        if (ctx->data.warning_engine_temp) {
            draw_text_ttf(ctx->renderer, ctx->font_arial_bold, "HIGH ENGINE TEMP", warn_x + warn_w / 2, msg_y, COLOR_POLARIS_RED, true);
            msg_y += 30;
        }
        if (ctx->data.warning_belt_temp) {
            draw_text_ttf(ctx->renderer, ctx->font_arial_bold, "BELT TEMP HIGH!", warn_x + warn_w / 2, msg_y, COLOR_POLARIS_RED, true);
            msg_y += 30;
        }
        if (ctx->data.warning_low_fuel) {
            draw_text_ttf(ctx->renderer, ctx->font_arial_bold, "LOW FUEL", warn_x + warn_w / 2, msg_y, COLOR_POLARIS_AMBER, true);
            msg_y += 30;
        }
        if (ctx->data.warning_low_voltage) {
            draw_text_ttf(ctx->renderer, ctx->font_arial_bold, "LOW VOLTAGE", warn_x + warn_w / 2, msg_y, COLOR_POLARIS_AMBER, true);
        }
    }
    
    SDL_RenderPresent(ctx->renderer);
}

void draw_gauge(AppContext *ctx, int cx, int cy, int radius, float value, float max_value, bool is_primary) {
    (void)is_primary;  // Unused but kept for API compatibility
    // Background circle (glass panel)
    SDL_SetRenderDrawColor(ctx->renderer, COLOR_GLASS.r, COLOR_GLASS.g, COLOR_GLASS.b, COLOR_GLASS.a);
    for (int i = 0; i < 15; i++) {
        draw_circle(ctx->renderer, cx, cy, radius + i);
    }
    
    // Border (more visible)
    SDL_SetRenderDrawColor(ctx->renderer, COLOR_BORDER.r, COLOR_BORDER.g, COLOR_BORDER.b, COLOR_BORDER.a);
    draw_circle(ctx->renderer, cx, cy, radius + 15);
    draw_circle(ctx->renderer, cx, cy, radius + 16);
    draw_circle(ctx->renderer, cx, cy, radius + 17);
    
    // Background arc track (always visible, darker)
    SDL_SetRenderDrawColor(ctx->renderer, COLOR_GAUGE_BG.r, COLOR_GAUGE_BG.g, COLOR_GAUGE_BG.b, COLOR_GAUGE_BG.a);
    draw_arc(ctx->renderer, cx, cy, radius, -225, 45, 15);
    
    // Progress arc
    float percentage = fminf(value / max_value, 1.0f);
    Color arc_color = COLOR_PRIMARY;
    
    // Color changes based on percentage
    if (percentage > 0.9f) {
        arc_color = COLOR_WARNING;  // Red when near max
    } else if (percentage > 0.75f) {
        arc_color = COLOR_ACCENT;   // Orange in high range
    } else {
        // Make the arc brighter for better visibility
        arc_color.r = COLOR_PRIMARY.r;
        arc_color.g = COLOR_PRIMARY.g + 20;  // Brighter cyan
        arc_color.b = 255;
    }
    
    SDL_SetRenderDrawColor(ctx->renderer, arc_color.r, arc_color.g, arc_color.b, 255);
    
    // Draw filled arc (thick and bright)
    float start_angle = -225.0f * M_PI / 180.0f;
    float sweep_angle = 270.0f * percentage * M_PI / 180.0f;
    
    // Draw arc with multiple layers for solid, thick fill
    for (int thickness = 0; thickness < 15; thickness++) {
        int r = radius - 7 + thickness;
        int num_segments = (int)(270 * percentage * 2);  // More segments for smoother arc
        if (num_segments < 2) num_segments = 2;
        for (int i = 0; i <= num_segments; i++) {
            float angle = start_angle + (sweep_angle * i / num_segments);
            float px = cx + r * cosf(angle);
            float py = cy + r * sinf(angle);
            // Draw multiple points for better coverage
            SDL_RenderPoint(ctx->renderer, px, py);
            SDL_RenderPoint(ctx->renderer, px + 1, py);
            SDL_RenderPoint(ctx->renderer, px, py + 1);
            SDL_RenderPoint(ctx->renderer, px + 1, py + 1);
        }
    }
    
    // NO center dot - it overlaps with numbers!
    
    (void)value;  // Value displayed separately with draw_number
}


void draw_filled_circle(SDL_Renderer *renderer, int cx, int cy, int radius) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) {
                SDL_RenderPoint(renderer, (float)(cx + x), (float)(cy + y));
            }
        }
    }
}

void draw_circle(SDL_Renderer *renderer, int cx, int cy, int radius) {
    int x = radius;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        SDL_RenderPoint(renderer, (float)(cx + x), (float)(cy + y));
        SDL_RenderPoint(renderer, (float)(cx + y), (float)(cy + x));
        SDL_RenderPoint(renderer, (float)(cx - y), (float)(cy + x));
        SDL_RenderPoint(renderer, (float)(cx - x), (float)(cy + y));
        SDL_RenderPoint(renderer, (float)(cx - x), (float)(cy - y));
        SDL_RenderPoint(renderer, (float)(cx - y), (float)(cy - x));
        SDL_RenderPoint(renderer, (float)(cx + y), (float)(cy - x));
        SDL_RenderPoint(renderer, (float)(cx + x), (float)(cy - y));
        
        if (err <= 0) {
            y += 1;
            err += 2*y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2*x + 1;
        }
    }
}

void draw_arc(SDL_Renderer *renderer, int cx, int cy, int radius, float start_angle, float end_angle, int thickness) {
    float start_rad = start_angle * M_PI / 180.0f;
    float end_rad = end_angle * M_PI / 180.0f;
    
    int num_segments = (int)(fabs(end_angle - start_angle) * 2);
    float angle_step = (end_rad - start_rad) / num_segments;
    
    for (int i = 0; i < num_segments; i++) {
        float angle1 = start_rad + i * angle_step;
        float angle2 = start_rad + (i + 1) * angle_step;
        
        for (int t = 0; t < thickness; t++) {
            int r = radius + t - thickness / 2;
            float x1 = cx + r * cosf(angle1);
            float y1 = cy + r * sinf(angle1);
            float x2 = cx + r * cosf(angle2);
            float y2 = cy + r * sinf(angle2);
            
            SDL_RenderLine(renderer, x1, y1, x2, y2);
        }
    }
}

void draw_rounded_rect(SDL_Renderer *renderer, int x, int y, int w, int h, int radius) {
    // Top line
    SDL_RenderLine(renderer, (float)(x + radius), (float)y, (float)(x + w - radius), (float)y);
    // Bottom line
    SDL_RenderLine(renderer, (float)(x + radius), (float)(y + h), (float)(x + w - radius), (float)(y + h));
    // Left line
    SDL_RenderLine(renderer, (float)x, (float)(y + radius), (float)x, (float)(y + h - radius));
    // Right line
    SDL_RenderLine(renderer, (float)(x + w), (float)(y + radius), (float)(x + w), (float)(y + h - radius));
    
    // Corners (simplified)
    for (int i = 0; i < radius; i++) {
        float angle = i * M_PI / (2 * radius);
        int dx = (int)(radius * cosf(angle));
        int dy = (int)(radius * sinf(angle));
        
        SDL_RenderPoint(renderer, (float)(x + radius - dx), (float)(y + radius - dy));
        SDL_RenderPoint(renderer, (float)(x + w - radius + dx), (float)(y + radius - dy));
        SDL_RenderPoint(renderer, (float)(x + radius - dx), (float)(y + h - radius + dy));
        SDL_RenderPoint(renderer, (float)(x + w - radius + dx), (float)(y + h - radius + dy));
    }
}

// Draw a 7-segment style digit
void draw_digit(SDL_Renderer *renderer, int digit, int x, int y, int width, int height, Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    
    int seg_h = height / 2;
    int seg_w = width;
    int thickness = width / 5;
    
    // 7-segment layout: segments numbered 0-6
    // Segment positions: 0=top, 1=top-right, 2=bottom-right, 3=bottom, 4=bottom-left, 5=top-left, 6=middle
    bool segments[10][7] = {
        {1,1,1,1,1,1,0}, // 0
        {0,1,1,0,0,0,0}, // 1
        {1,1,0,1,1,0,1}, // 2
        {1,1,1,1,0,0,1}, // 3
        {0,1,1,0,0,1,1}, // 4
        {1,0,1,1,0,1,1}, // 5
        {1,0,1,1,1,1,1}, // 6
        {1,1,1,0,0,0,0}, // 7
        {1,1,1,1,1,1,1}, // 8
        {1,1,1,1,0,1,1}  // 9
    };
    
    if (digit < 0 || digit > 9) return;
    
    // Top
    if (segments[digit][0]) {
        SDL_FRect seg = {(float)(x + thickness), (float)y, (float)(seg_w - 2*thickness), (float)thickness};
        SDL_RenderFillRect(renderer, &seg);
    }
    // Top-right
    if (segments[digit][1]) {
        SDL_FRect seg = {(float)(x + seg_w - thickness), (float)(y + thickness), (float)thickness, (float)(seg_h - thickness)};
        SDL_RenderFillRect(renderer, &seg);
    }
    // Bottom-right
    if (segments[digit][2]) {
        SDL_FRect seg = {(float)(x + seg_w - thickness), (float)(y + seg_h), (float)thickness, (float)(seg_h - thickness)};
        SDL_RenderFillRect(renderer, &seg);
    }
    // Bottom
    if (segments[digit][3]) {
        SDL_FRect seg = {(float)(x + thickness), (float)(y + height - thickness), (float)(seg_w - 2*thickness), (float)thickness};
        SDL_RenderFillRect(renderer, &seg);
    }
    // Bottom-left
    if (segments[digit][4]) {
        SDL_FRect seg = {(float)x, (float)(y + seg_h), (float)thickness, (float)(seg_h - thickness)};
        SDL_RenderFillRect(renderer, &seg);
    }
    // Top-left
    if (segments[digit][5]) {
        SDL_FRect seg = {(float)x, (float)(y + thickness), (float)thickness, (float)(seg_h - thickness)};
        SDL_RenderFillRect(renderer, &seg);
    }
    // Middle
    if (segments[digit][6]) {
        SDL_FRect seg = {(float)(x + thickness), (float)(y + seg_h - thickness/2), (float)(seg_w - 2*thickness), (float)thickness};
        SDL_RenderFillRect(renderer, &seg);
    }
}

// Draw a multi-digit number
void draw_number(SDL_Renderer *renderer, int value, int x, int y, int size, Color color) {
    if (value < 0) value = 0;
    if (value > 9999) value = 9999;
    
    char num_str[16];
    snprintf(num_str, sizeof(num_str), "%d", value);
    int len = strlen(num_str);
    
    int digit_width = size * 6 / 10;
    int digit_height = size;
    int spacing = size / 5;
    
    for (int i = 0; i < len; i++) {
        int digit = num_str[i] - '0';
        draw_digit(renderer, digit, x + i * (digit_width + spacing), y, digit_width, digit_height, color);
    }
}

// Draw text using TTF fonts
void draw_text_ttf(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y, Color color, bool centered) {
    if (!font || !text) return;
    
    SDL_Color sdl_color = {color.r, color.g, color.b, color.a};
    SDL_Surface *surface = TTF_RenderText_Blended(font, text, 0, sdl_color);
    if (!surface) return;
    
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_DestroySurface(surface);
        return;
    }
    
    SDL_FRect dest = {(float)x, (float)y, (float)surface->w, (float)surface->h};
    if (centered) {
        dest.x -= surface->w / 2.0f;
        dest.y -= surface->h / 2.0f;
    }
    
    SDL_RenderTexture(renderer, texture, NULL, &dest);
    
    SDL_DestroyTexture(texture);
    SDL_DestroySurface(surface);
}

// Draw simple text labels using rectangles (fallback)
void draw_label(SDL_Renderer *renderer, const char *text, int x, int y, int size, Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    
    int char_width = size * 5 / 8;
    int char_height = size;
    int spacing = size / 4;
    
    // Simple block letter rendering for basic labels
    for (int i = 0; text[i] != '\0'; i++) {
        int cx = x + i * (char_width + spacing);
        
        // Draw a simple rectangle for each character (placeholder)
        SDL_FRect rect = {(float)cx, (float)y, (float)char_width, (float)char_height};
        SDL_RenderRect(renderer, &rect);
    }
}

// Draw drive mode indicator
void draw_drive_mode(AppContext *ctx, int x, int y, int size) {
    DriveMode mode = ctx->data.drive_mode;
    Color mode_color;
    const char *mode_text;
    
    switch (mode) {
        case MODE_DRIVE:
            mode_color = COLOR_SUCCESS;
            mode_text = "D";
            break;
        case MODE_REVERSE:
        default:
            mode_color = COLOR_WARNING;
            mode_text = "R";
            break;
    }
    
    // Draw using TTF font
    draw_text_ttf(ctx->renderer, ctx->font_arial_bold, mode_text, x, y, mode_color, true);
    
    // Background circle
    SDL_SetRenderDrawColor(ctx->renderer, COLOR_GLASS.r, COLOR_GLASS.g, COLOR_GLASS.b, 100);
    draw_filled_circle(ctx->renderer, x, y, size);
    SDL_SetRenderDrawColor(ctx->renderer, mode_color.r, mode_color.g, mode_color.b, 255);
    draw_circle(ctx->renderer, x, y, size);
    
    return;  // TTF rendered, skip old shape drawing
    
    // Draw large letter for mode (old code - not reached)
    SDL_SetRenderDrawColor(ctx->renderer, mode_color.r, mode_color.g, mode_color.b, 255);
    
    // Draw D, R, or N using simple shapes (old fallback code - not reached)
    int w = size;
    int h = size * 2;
    int thickness = size / 5;
    
    if (mode == MODE_DRIVE) {
        // Draw D shape
        SDL_FRect left = {(float)(x - w/2), (float)(y - h/2), (float)thickness, (float)h};
        SDL_RenderFillRect(ctx->renderer, &left);
        SDL_FRect top = {(float)(x - w/2), (float)(y - h/2), (float)w, (float)thickness};
        SDL_RenderFillRect(ctx->renderer, &top);
        SDL_FRect bottom = {(float)(x - w/2), (float)(y + h/2 - thickness), (float)w, (float)thickness};
        SDL_RenderFillRect(ctx->renderer, &bottom);
        // Right curve (simplified)
        for (int i = 0; i < h; i++) {
            int offset = (int)(w * 0.3 * sinf(M_PI * i / h));
            SDL_RenderPoint(ctx->renderer, (float)(x + w/2 - offset), (float)(y - h/2 + i));
        }
    } else if (mode == MODE_REVERSE) {
        // Draw R shape
        SDL_FRect left = {(float)(x - w/2), (float)(y - h/2), (float)thickness, (float)h};
        SDL_RenderFillRect(ctx->renderer, &left);
        SDL_FRect top = {(float)(x - w/2), (float)(y - h/2), (float)w, (float)thickness};
        SDL_RenderFillRect(ctx->renderer, &top);
        SDL_FRect mid = {(float)(x - w/2), (float)y, (float)w, (float)thickness};
        SDL_RenderFillRect(ctx->renderer, &mid);
        SDL_FRect right_top = {(float)(x + w/2 - thickness), (float)(y - h/2), (float)thickness, (float)h/2};
        SDL_RenderFillRect(ctx->renderer, &right_top);
        // Diagonal leg
        for (int i = 0; i < h/2; i++) {
            SDL_RenderPoint(ctx->renderer, (float)(x + i * thickness / 10), (float)(y + i));
        }
    } else {
        // Draw N shape
        SDL_FRect left = {(float)(x - w/2), (float)(y - h/2), (float)thickness, (float)h};
        SDL_RenderFillRect(ctx->renderer, &left);
        SDL_FRect right = {(float)(x + w/2 - thickness), (float)(y - h/2), (float)thickness, (float)h};
        SDL_RenderFillRect(ctx->renderer, &right);
        // Diagonal
        for (int i = 0; i < h; i++) {
            SDL_RenderLine(ctx->renderer, (float)(x - w/2), (float)(y - h/2 + i), 
                          (float)(x + w/2), (float)(y - h/2 + i));
        }
    }
    
    (void)mode_text;  // Not used in simple rendering
}

// Boot screen animation
void draw_boot_screen(AppContext *ctx) {
    Uint32 elapsed = SDL_GetTicks() - ctx->boot_start_time;
    float progress = fminf(elapsed / 3000.0f, 1.0f);
    
    // Draw SNOW-PI logo
    int center_x = WINDOW_WIDTH / 2;
    int center_y = WINDOW_HEIGHT / 2;
    
    // Animated circle
    SDL_SetRenderDrawColor(ctx->renderer, COLOR_PRIMARY.r, COLOR_PRIMARY.g, COLOR_PRIMARY.b, 255);
    int circle_radius = (int)(100 * progress);
    draw_circle(ctx->renderer, center_x, center_y, circle_radius);
    
    // Boot text using TTF
    if (progress > 0.3f) {
        draw_text_ttf(ctx->renderer, ctx->font_arial_bold, "SNOW-PI", center_x, center_y - 150, COLOR_PRIMARY, true);
    }
    
    if (progress > 0.5f) {
        draw_text_ttf(ctx->renderer, ctx->font_arial_small, "Pi-Dash", center_x, center_y, COLOR_SUCCESS, true);
    }
    
    // Progress bar
    if (progress > 0.2f) {
        int bar_w = 300;
        int bar_h = 10;
        int bar_x = center_x - bar_w / 2;
        int bar_y = center_y + 120;
        
        SDL_SetRenderDrawColor(ctx->renderer, COLOR_BORDER.r, COLOR_BORDER.g, COLOR_BORDER.b, 255);
        SDL_FRect bar_bg = {(float)bar_x, (float)bar_y, (float)bar_w, (float)bar_h};
        SDL_RenderRect(ctx->renderer, &bar_bg);
        
        SDL_SetRenderDrawColor(ctx->renderer, COLOR_PRIMARY.r, COLOR_PRIMARY.g, COLOR_PRIMARY.b, 255);
        SDL_FRect bar_fill = {(float)bar_x, (float)bar_y, bar_w * progress, (float)bar_h};
        SDL_RenderFillRect(ctx->renderer, &bar_fill);
    }
    
    // Hint text
    if (progress > 0.7f) {
        draw_text_ttf(ctx->renderer, ctx->font_arial_small, "PRESS SPACE TO SKIP", center_x, WINDOW_HEIGHT - 50, COLOR_SUCCESS, true);
    }
}

void draw_filled_rounded_rect(SDL_Renderer *renderer, int x, int y, int w, int h, int radius) {
    // Main rectangle
    SDL_FRect center = {(float)(x + radius), (float)y, (float)(w - 2 * radius), (float)h};
    SDL_RenderFillRect(renderer, &center);
    
    SDL_FRect left = {(float)x, (float)(y + radius), (float)radius, (float)(h - 2 * radius)};
    SDL_RenderFillRect(renderer, &left);
    
    SDL_FRect right = {(float)(x + w - radius), (float)(y + radius), (float)radius, (float)(h - 2 * radius)};
    SDL_RenderFillRect(renderer, &right);
    
    // Corners (simplified filled circles)
    for (int cy = 0; cy < radius; cy++) {
        for (int cx = 0; cx < radius; cx++) {
            if (cx*cx + cy*cy <= radius*radius) {
                SDL_RenderPoint(renderer, (float)(x + radius - cx), (float)(y + radius - cy));
                SDL_RenderPoint(renderer, (float)(x + w - radius + cx), (float)(y + radius - cy));
                SDL_RenderPoint(renderer, (float)(x + radius - cx), (float)(y + h - radius + cy));
                SDL_RenderPoint(renderer, (float)(x + w - radius + cx), (float)(y + h - radius + cy));
            }
        }
    }
}

