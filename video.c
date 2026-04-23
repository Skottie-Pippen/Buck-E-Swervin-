#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include "defines.h"
#include "sprites.h"
#include "audio.h"

typedef enum { STATE_SETUP, STATE_PLAY, STATE_GAMEOVER } GameState;

typedef struct {
    int lane;
    int y;
    bool active;
} Obstacle;

#define MAX_CMD_LEN        96
#define TITLE_LEFT         20
#define TITLE_RIGHT        300
#define TITLE_TOP          90
#define TITLE_BOTTOM       210
#define PREVIEW_OFFSET_Y   40
#define YARD_LINE_SPACING  60

static GameState current_state = STATE_SETUP;
static volatile bool stop_game = false;
static volatile int score = 0;
static volatile int lives = MAX_LIVES;
int game_speed = SPEED_EASY;
int player_lane = PLAYER_START_LANE;
static int player_x = 0;
static int player_y = PLAYER_START_Y;
static int video_FD = -1;
static int scroll_offset = 0;
static int field_offset_yards = 10;
static int num_active_obstacles = 0;
static int current_difficulty = 0;
static int animation_frame = 0;
static int high_score = 0;
static Obstacle obstacles[MAX_OBSTACLES];

static const char *difficulty_labels[4] = { "EASY", "MED", "HARD", "XTREME" };

#define HIGHSCORE_FILE "./highscore.txt"

static void video_command(const char *cmd);
static void draw_box_rgb(int x1, int y1, int x2, int y2, uint16_t color);
static void draw_line_rgb(int x1, int y1, int x2, int y2, uint16_t color);
static void draw_pixel_rgb(int x, int y, uint16_t color);
static void draw_field_background(bool advance_scroll);
static void draw_digit_oriented(int base_x, int base_y, int digit, int orientation);
static void draw_sprite(int x, int y, const Sprite *sprite);
static void draw_player(void);
static void draw_obstacles(void);
static void draw_hud_panel(void);
static void draw_life_icons(void);
static void draw_title_screen(void);
static void draw_gameplay_scene(void);
static void draw_game_over_screen(void);
static void draw_title_text(void);
static void draw_game_over_text(void);
static void update_hud_text(bool force);
static void update_game_state(void);
static bool check_collision(void);
static void spawn_obstacle(void);
static void reset_obstacles(void);
static void seed_rng(void);
static void load_high_score(void);
static void save_high_score(void);

void cleanup_and_exit(void);
void init_game_state(void);
void pushbutton(int press);
void update_game_difficulty(int sw_state);
int read_driver_input(const char *dev_path);
void draw_text_on_screen(int x, int y, char *text);
void draw_centered_text(int y, char *text);
void clear_text_layer(void);

static inline int lane_to_x_center(int lane) {
    return GAME_X_OFFSET + (LANE_WIDTH / 2) + (lane * LANE_WIDTH);
}

static void video_command(const char *cmd) {
    if (video_FD >= 0) {
        write(video_FD, cmd, strlen(cmd));
    }
}

static void draw_box_rgb(int x1, int y1, int x2, int y2, uint16_t color) {
    if (x2 < x1) { int tmp = x1; x1 = x2; x2 = tmp; }
    if (y2 < y1) { int tmp = y1; y1 = y2; y2 = tmp; }
    char cmd[MAX_CMD_LEN];
    snprintf(cmd, sizeof(cmd), "box %d,%d %d,%d %04X", x1, y1, x2, y2, color);
    video_command(cmd);
}

static void draw_line_rgb(int x1, int y1, int x2, int y2, uint16_t color) {
    char cmd[MAX_CMD_LEN];
    snprintf(cmd, sizeof(cmd), "line %d,%d %d,%d %04X", x1, y1, x2, y2, color);
    video_command(cmd);
}

static void draw_pixel_rgb(int x, int y, uint16_t color) {
    char cmd[MAX_CMD_LEN];
    snprintf(cmd, sizeof(cmd), "pixel %d,%d %04X", x, y, color);
    video_command(cmd);
}

static void draw_digit_oriented(int base_x, int base_y, int digit, int orientation) {
    const uint16_t col = WHITE;
    const int w = 12;
    const int h = 20;

#define DL(x1, y1, x2, y2)                                                      \
    do {                                                                        \
        if (orientation == 1) {                                                 \
            draw_line_rgb(base_x + (y1), base_y + (x1),                          \
                         base_x + (y2), base_y + (x2), col);                    \
        } else {                                                                \
            draw_line_rgb(base_x - (y1), base_y + (x1),                          \
                         base_x - (y2), base_y + (x2), col);                    \
        }                                                                       \
    } while (0)

    switch (digit) {
        case 0: DL(0, 0, w, 0); DL(0, h, w, h); DL(0, 0, 0, h); DL(w, 0, w, h); break;
        case 1: DL(w / 2, 0, w / 2, h); DL(w / 2 - 3, 0, w / 2 + 3, 0);
                DL(w / 2 - 3, h, w / 2 + 3, h); break;
        case 2: DL(0, 0, w, 0); DL(w, 0, w, h / 2); DL(0, h / 2, w, h / 2);
                DL(0, h / 2, 0, h); DL(0, h, w, h); break;
        case 3: DL(0, 0, w, 0); DL(w, 0, w, h); DL(0, h / 2, w, h / 2);
                DL(0, h, w, h); break;
        case 4: DL(0, 0, 0, h / 2); DL(0, h / 2, w, h / 2); DL(w, 0, w, h); break;
        case 5: DL(w, 0, 0, 0); DL(0, 0, 0, h / 2); DL(0, h / 2, w, h / 2);
                DL(w, h / 2, w, h); DL(w, h, 0, h); break;
        default: break;
    }
#undef DL
}

static void draw_field_background(bool advance_scroll) {
    int i, j;
    const int field_left = GAME_X_OFFSET;
    const int field_right = GAME_X_OFFSET + (NUM_LANES * LANE_WIDTH);
    const int pixels_per_yard = YARD_LINE_SPACING / 5;

    draw_box_rgb(field_left, HUD_HEIGHT, field_right, SCREEN_HEIGHT - 1, FIELD_COLOR);

    // Only draw the sidelines (left and right borders), no internal lane lines
    draw_line_rgb(field_left, HUD_HEIGHT, field_left, SCREEN_HEIGHT - 1, WHITE);
    draw_line_rgb(field_right, HUD_HEIGHT, field_right, SCREEN_HEIGHT - 1, WHITE);

    for (i = -1; i < (SCREEN_HEIGHT / YARD_LINE_SPACING) + 2; ++i) {
        int y = HUD_HEIGHT + (i * YARD_LINE_SPACING) + scroll_offset;
        int yard_val = field_offset_yards - (i * 5);
        int effective = yard_val % 100;
        if (effective < 0) effective += 100;
        if (effective > 50) effective = 100 - effective;

        // Draw the main 5-yard line
        if (y >= HUD_HEIGHT && y < SCREEN_HEIGHT) {
            draw_line_rgb(field_left, y, field_right, y, WHITE);
            if (effective > 0 && (effective % 10) == 0) {
                int tens_digit = effective / 10;
                int ones_digit = effective % 10;
                
                // Left side tens (top)
                if (y - 15 >= HUD_HEIGHT && y + 5 < SCREEN_HEIGHT) {
                    draw_digit_oriented(field_left + 18, y - 15, tens_digit, 1);
                }
                // Left side ones (bottom)
                if (y + 7 >= HUD_HEIGHT && y + 27 < SCREEN_HEIGHT) {
                    draw_digit_oriented(field_left + 18, y + 7, ones_digit, 1);
                }
                
                // Right side tens (bottom)
                if (y + 7 >= HUD_HEIGHT && y + 27 < SCREEN_HEIGHT) {
                    draw_digit_oriented(field_right - 18, y + 7, tens_digit, 2);
                }
                // Right side ones (top)
                if (y - 15 >= HUD_HEIGHT && y + 5 < SCREEN_HEIGHT) {
                    draw_digit_oriented(field_right - 18, y - 15, ones_digit, 2);
                }
            }
        }

        // Draw intermediate yard notches (4 notches between 5-yard lines)
        for (j = 1; j < 5; ++j) {
            int notch_y = y + (j * pixels_per_yard);
            if (notch_y >= HUD_HEIGHT && notch_y < SCREEN_HEIGHT) {
                // Sideline notches
                draw_line_rgb(field_left, notch_y, field_left + 5, notch_y, WHITE);
                draw_line_rgb(field_right - 5, notch_y, field_right, notch_y, WHITE);

                // Hash marks (approximate locations)
                int hash_x1 = field_left + (field_right - field_left) * 0.35;
                int hash_x2 = field_left + (field_right - field_left) * 0.65;
                draw_line_rgb(hash_x1, notch_y, hash_x1 + 5, notch_y, WHITE);
                draw_line_rgb(hash_x2, notch_y, hash_x2 + 5, notch_y, WHITE);
            }
        }
    }

    if (advance_scroll) {
        scroll_offset += game_speed;
        if (scroll_offset >= YARD_LINE_SPACING) {
            scroll_offset -= YARD_LINE_SPACING;
            field_offset_yards = (field_offset_yards + 5) % 100;
        }
    }
}

static void draw_sprite(int x, int y, const Sprite *sprite) {
    int i;
    for (i = 0; i < sprite->num_rects; ++i) {
        const SpriteRect *r = &sprite->rects[i];
        draw_box_rgb(x + r->x_offset, y + r->y_offset, 
                     x + r->x_offset + r->width, y + r->y_offset + r->height, 
                     r->color);
    }
}

static void draw_player(void) {
    player_x = lane_to_x_center(player_lane) - (PLAYER_SIZE / 2);
    int frame_idx = (animation_frame / 4) % 4;
    draw_sprite(player_x, player_y, &buck_e_frames[frame_idx]);
}

static void draw_obstacles(void) {
    int i;
    int frame_idx = (animation_frame / 4) % 4;
    for (i = 0; i < MAX_OBSTACLES; ++i) {
        if (!obstacles[i].active) continue;
        int ox = lane_to_x_center(obstacles[i].lane) - (OBSTACLE_WIDTH / 2);
        draw_sprite(ox, obstacles[i].y, &defender_frames[frame_idx]);
    }
}

static void draw_life_icons(void) {
    const int icon_width = 18;
    const int icon_height = 10;
    int start_x = 10;
    int top = 8;
    int i;
    for (i = 0; i < MAX_LIVES; ++i) {
        uint16_t fill = (i < lives) ? RED : HUD_BG_COLOR;
        int left = start_x + i * (icon_width + 6);
        draw_box_rgb(left, top, left + icon_width, top + icon_height, fill);
        draw_line_rgb(left, top, left + icon_width, top, WHITE);
        draw_line_rgb(left, top + icon_height, left + icon_width, top + icon_height, WHITE);
        draw_line_rgb(left, top, left, top + icon_height, WHITE);
        draw_line_rgb(left + icon_width, top, left + icon_width, top + icon_height, WHITE);
    }
}

static void draw_hud_panel(void) {
    draw_box_rgb(0, 0, SCREEN_WIDTH - 1, HUD_HEIGHT, HUD_BG_COLOR);
    draw_line_rgb(0, HUD_HEIGHT, SCREEN_WIDTH - 1, HUD_HEIGHT, WHITE);
    draw_life_icons();
}

static void draw_title_screen(void) {
    draw_field_background(false);
    draw_hud_panel();

    draw_box_rgb(TITLE_LEFT, TITLE_TOP, TITLE_RIGHT, TITLE_BOTTOM, OVERLAY_BG_COLOR);
    draw_line_rgb(TITLE_LEFT, TITLE_TOP, TITLE_RIGHT, TITLE_TOP, WHITE);
    draw_line_rgb(TITLE_LEFT, TITLE_BOTTOM, TITLE_RIGHT, TITLE_BOTTOM, WHITE);
    draw_line_rgb(TITLE_LEFT, TITLE_TOP, TITLE_LEFT, TITLE_BOTTOM, WHITE);
    draw_line_rgb(TITLE_RIGHT, TITLE_TOP, TITLE_RIGHT, TITLE_BOTTOM, WHITE);

    int preview_x = lane_to_x_center(PLAYER_START_LANE) - (PLAYER_SIZE / 2);
    draw_sprite(preview_x, TITLE_BOTTOM - PREVIEW_OFFSET_Y - PLAYER_SIZE, &buck_e_frames[0]);

    int defender_lane = (PLAYER_START_LANE + 2) % NUM_LANES;
    int defender_x = lane_to_x_center(defender_lane) - (OBSTACLE_WIDTH / 2);
    draw_sprite(defender_x, TITLE_TOP + 30, &defender_frames[0]);
}

static void draw_gameplay_scene(void) {
    draw_field_background(true);
    draw_obstacles();
    draw_player();
    draw_hud_panel();
}

static void draw_game_over_screen(void) {
    draw_field_background(false);
    draw_hud_panel();
    draw_box_rgb(TITLE_LEFT, TITLE_TOP, TITLE_RIGHT, TITLE_BOTTOM, OVERLAY_BG_COLOR);
    draw_line_rgb(TITLE_LEFT, TITLE_TOP, TITLE_RIGHT, TITLE_TOP, WHITE);
    draw_line_rgb(TITLE_LEFT, TITLE_BOTTOM, TITLE_RIGHT, TITLE_BOTTOM, WHITE);
    draw_line_rgb(TITLE_LEFT, TITLE_TOP, TITLE_LEFT, TITLE_BOTTOM, WHITE);
    draw_line_rgb(TITLE_RIGHT, TITLE_TOP, TITLE_RIGHT, TITLE_BOTTOM, WHITE);
}

static void draw_title_text(void) {
    char line[40];
    clear_text_layer();
    draw_centered_text(12, "BUCK-E SWERVIN");
    draw_centered_text(14, "Dodge defenders and rack up yards!");
    snprintf(line, sizeof(line), "HIGH SCORE: %d", high_score);
    draw_centered_text(16, line);
    draw_centered_text(18, "KEY0: Move RIGHT / Start Game");
    draw_centered_text(19, "KEY1: Move LEFT");
    draw_centered_text(21, "SW[1:0] selects difficulty");
    draw_centered_text(24, "Press KEY0 to begin");
}

static void draw_game_over_text(void) {
    char line[40];
    clear_text_layer();
    draw_centered_text(14, "GAME OVER");
    snprintf(line, sizeof(line), "FINAL SCORE: %d", score);
    draw_centered_text(16, line);
    if (score > high_score) {
        draw_centered_text(17, "*** NEW HIGH SCORE! ***");
    }
    snprintf(line, sizeof(line), "HIGH SCORE: %d", high_score);
    draw_centered_text(18, line);
    draw_centered_text(21, "Press KEY0 to play again");
}

static void update_hud_text(bool force) {
    static int last_score = -1;
    static int last_lives = -1;
    static int last_diff = -1;
    static int last_high_score = -1;
    char line[40];

    if (force || last_score != score) {
        snprintf(line, sizeof(line), "SCORE: %04d", score);
        draw_text_on_screen(52, 1, line);
        last_score = score;
    }

    if (force || last_lives != lives) {
        snprintf(line, sizeof(line), "LIVES: %d", lives);
        draw_text_on_screen(0, 1, line);
        last_lives = lives;
    }

    if (force || last_diff != current_difficulty) {
        snprintf(line, sizeof(line), "DIFF: %s", difficulty_labels[current_difficulty]);
        draw_text_on_screen(52, 2, line);
        last_diff = current_difficulty;
    }

    if (force || last_high_score != high_score) {
        snprintf(line, sizeof(line), "HI: %04d", high_score);
        draw_text_on_screen(68, 2, line);
        last_high_score = high_score;
    }
}

static void reset_obstacles(void) {
    int i;
    for (i = 0; i < MAX_OBSTACLES; ++i) {
        obstacles[i].active = false;
    }
    num_active_obstacles = 0;
}

void init_game_state(void) {
    score = 0;
    lives = MAX_LIVES;
    player_lane = PLAYER_START_LANE;
    player_y = PLAYER_START_Y;
    scroll_offset = 0;
    field_offset_yards = 10;
    reset_obstacles();
}

static void spawn_obstacle(void) {
    int i, j;
    int spawn_y = HUD_HEIGHT - OBSTACLE_HEIGHT;
    int min_spacing = 50; // Minimum vertical spacing between obstacles in same lane
    
    // Find an inactive obstacle slot
    for (i = 0; i < MAX_OBSTACLES; ++i) {
        if (!obstacles[i].active) {
            // Try to find a lane that doesn't have an obstacle near spawn point
            int attempts = 0;
            int chosen_lane;
            bool lane_clear;
            
            do {
                chosen_lane = rand() % NUM_LANES;
                lane_clear = true;
                
                // Check if any active obstacle is in this lane near the spawn point
                for (j = 0; j < MAX_OBSTACLES; ++j) {
                    if (obstacles[j].active && 
                        obstacles[j].lane == chosen_lane &&
                        ABS(obstacles[j].y - spawn_y) < min_spacing) {
                        lane_clear = false;
                        break;
                    }
                }
                
                attempts++;
                // Give up after 10 attempts to avoid infinite loop
                if (attempts >= 10) {
                    lane_clear = true; // Force spawn anyway
                }
            } while (!lane_clear);
            
            obstacles[i].lane = chosen_lane;
            obstacles[i].y = spawn_y;
            obstacles[i].active = true;
            num_active_obstacles++;
            return;
        }
    }
}

static void update_game_state(void) {
    int i;
    animation_frame++;
    for (i = 0; i < MAX_OBSTACLES; ++i) {
        if (!obstacles[i].active) continue;
        obstacles[i].y += game_speed;
        if (obstacles[i].y > SCREEN_HEIGHT) {
            obstacles[i].active = false;
            if (num_active_obstacles > 0) num_active_obstacles--;
            score++;
            audio_play_effect(SOUND_SCORE);
        }
    }

    int spawn_window = (game_speed > 0) ? (80 / game_speed) : 20;
    if (spawn_window < 3) spawn_window = 3;
    if ((rand() % spawn_window) == 0 && num_active_obstacles < MAX_OBSTACLES) {
        spawn_obstacle();
    }
}

static bool check_collision(void) {
    int i;
    player_x = lane_to_x_center(player_lane) - (PLAYER_SIZE / 2);
    int px2 = player_x + PLAYER_SIZE;
    int py2 = player_y + PLAYER_SIZE;

    for (i = 0; i < MAX_OBSTACLES; ++i) {
        if (!obstacles[i].active) continue;
        int ox = lane_to_x_center(obstacles[i].lane) - (OBSTACLE_WIDTH / 2);
        int oy = obstacles[i].y;
        int ox2 = ox + OBSTACLE_WIDTH;
        int oy2 = oy + OBSTACLE_HEIGHT;
        if (!(player_x > ox2 || px2 < ox || player_y > oy2 || py2 < oy)) {
            obstacles[i].active = false;
            if (num_active_obstacles > 0) num_active_obstacles--;
            return true;
        }
    }
    return false;
}

static void load_high_score(void) {
    FILE *fp = fopen(HIGHSCORE_FILE, "r");
    if (fp == NULL) {
        high_score = 0;
        return;
    }
    if (fscanf(fp, "%d", &high_score) != 1) {
        high_score = 0;
    }
    fclose(fp);
}

static void save_high_score(void) {
    FILE *fp = fopen(HIGHSCORE_FILE, "w");
    if (fp == NULL) {
        fprintf(stderr, "Warning: Could not save high score to %s\n", HIGHSCORE_FILE);
        return;
    }
    fprintf(fp, "%d\n", high_score);
    fclose(fp);
}

static void seed_rng(void) {
    static bool seeded = false;
    if (!seeded) {
        srand(time(NULL));
        seeded = true;
    }
}

void signal_handler(int sig) {
    (void)sig;
    stop_game = true;
}

void cleanup_and_exit(void) {
    audio_close();
    if (video_FD >= 0) {
        video_command("clear");
        video_command("sync");
        video_command("erase");
        close(video_FD);
    }
    exit(0);
}

int read_driver_input(const char *dev_path) {
    char buffer[16];
    int fd = open(dev_path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) return 0;

    ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);
    if (bytes <= 0) {
        return 0;
    }

    bool looks_text = true;
    for (ssize_t i = 0; i < bytes; ++i) {
        unsigned char c = (unsigned char)buffer[i];
        if (!(isdigit(c) || isspace(c) || c == '-')) {
            looks_text = false;
            break;
        }
    }

    if (looks_text) {
        buffer[bytes] = '\0';
        return atoi(buffer);
    }

    int value = 0;
    size_t copy_bytes = (bytes < (ssize_t)sizeof(value)) ? (size_t)bytes : sizeof(value);
    memcpy(&value, buffer, copy_bytes);
    return value;
}

void draw_text_on_screen(int x, int y, char *text) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "text %d,%d %s", x, y, text);
    video_command(cmd);
}

void draw_centered_text(int y, char *text) {
    int x = (80 - (int)strlen(text)) / 2;
    draw_text_on_screen(x, y, text);
}

void clear_text_layer(void) {
    video_command("erase");
}

int main(void) {
    int key_state = 0;
    int prev_key_state = 0;
    int sw_state = 0;
    bool just_changed = true;

    if (signal(SIGINT, signal_handler) == SIG_ERR) return 1;
    if ((video_FD = open("/dev/video", O_RDWR)) < 0) {
        perror("Error opening /dev/video");
        return 1;
    }

    if (!audio_init()) {
        fprintf(stderr, "Warning: Audio initialization failed. Continuing without audio.\n");
    }

    seed_rng();
    load_high_score();
    init_game_state();

    video_command("clear");
    video_command("sync");
    video_command("clear");
    clear_text_layer();

    while (!stop_game) {
        video_command("sync");

        sw_state = read_driver_input("/dev/IntelFPGAUP/SW");
        key_state = read_driver_input("/dev/IntelFPGAUP/KEY");
        update_game_difficulty(sw_state);
        current_difficulty = sw_state & 0x3;

        bool key0_edge = ((key_state & KEY0) && !(prev_key_state & KEY0));
        prev_key_state = key_state;

        switch (current_state) {
            case STATE_SETUP:
                if (just_changed) {
                    draw_title_text();
                    just_changed = false;
                }
                draw_title_screen();
                if (key0_edge) {
                    audio_play_effect(SOUND_WHISTLE);
                    init_game_state();
                    clear_text_layer();
                    current_state = STATE_PLAY;
                    just_changed = true;
                }
                break;

            case STATE_PLAY:
                if (just_changed) {
                    update_hud_text(true);
                    just_changed = false;
                }
                pushbutton(key_state);
                update_game_state();
                if (check_collision()) {
                    audio_play_effect(SOUND_COLLISION);
                    if (lives > 0) {
                        lives--;
                    }
                    if (lives <= 0) {
                        audio_play_effect(SOUND_GAMEOVER);
                        if (score > high_score) {
                            high_score = score;
                            save_high_score();
                        }
                        current_state = STATE_GAMEOVER;
                        just_changed = true;
                    }
                }
                draw_gameplay_scene();
                update_hud_text(false);
                break;

            case STATE_GAMEOVER:
                if (just_changed) {
                    draw_game_over_text();
                    just_changed = false;
                }
                draw_game_over_screen();
                if (key0_edge) {
                    init_game_state();
                    clear_text_layer();
                    current_state = STATE_PLAY;
                    just_changed = true;
                }
                break;
        }
    }

    cleanup_and_exit();
    return 0;
}