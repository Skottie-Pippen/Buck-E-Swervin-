#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "defines.h"

// --- Extern Global Variables from main.c ---
extern int player_lane;         
extern int game_speed;          

int get_speed_from_sw(int sw_value) {
    switch (sw_value) {
        case 0: return SPEED_EASY;
        case 1: return SPEED_MEDIUM;
        case 2: return SPEED_HARD;
        case 3: return SPEED_EXTREME;
        default: return SPEED_EASY; 
    }
}

void pushbutton(int press) {
    if (press & KEY0) {
        if (player_lane < NUM_LANES - 1) {
            player_lane++;
        }
    }
    
    if (press & KEY1) {
        if (player_lane > 0) {
            player_lane--;
        }
    }
}

void update_game_difficulty(int sw_state) {
    int difficulty_level = sw_state & 0b11; 
    game_speed = get_speed_from_sw(difficulty_level);
}