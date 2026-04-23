#ifndef SPRITES_H
#define SPRITES_H

#include <stdint.h>
#include "defines.h"

/*
 * Sprite Storage Format
 * Box Sprites for animation frames.
 */

typedef struct {
    int x_offset;
    int y_offset;
    int width;
    int height;
    uint16_t color;
} SpriteRect;

typedef struct {
    int num_rects;
    const SpriteRect *rects;
} Sprite;

/* ==========================================
   PLAYER SPRITES (Back View - Running Up)
   Colors: Yellow Helmet, Red Jersey, White Pants
   ========================================== */

// --- Frame 0: Neutral / Right Plant ---
static const SpriteRect p_f0_rects[] = {
    { 10, 0, 12, 10, YELLOW }, // Helmet
    { 8, 10, 16, 12, RED },    // Jersey
    { 4, 10, 4, 10, PINK },    // Left Arm
    { 24, 10, 4, 10, PINK },   // Right Arm
    { 8, 22, 6, 10, WHITE },   // Left Leg
    { 18, 22, 6, 10, WHITE },  // Right Leg
    { 8, 32, 6, 2, BLACK },    // Left Shoe
    { 18, 32, 6, 2, BLACK }    // Right Shoe
};

// --- Frame 1: Left Leg Up / Arms Swing ---
static const SpriteRect p_f1_rects[] = {
    { 10, 1, 12, 10, YELLOW }, // Helmet (Bob)
    { 8, 11, 16, 12, RED },    // Jersey
    { 4, 8, 4, 10, PINK },     // Left Arm (Up)
    { 24, 14, 4, 10, PINK },   // Right Arm (Down)
    { 8, 20, 6, 8, WHITE },    // Left Leg (Lifted)
    { 18, 23, 6, 10, WHITE },  // Right Leg (Plant)
    { 8, 28, 6, 2, BLACK },    // Left Shoe (Lifted)
    { 18, 33, 6, 2, BLACK }    // Right Shoe
};

// --- Frame 2: Neutral / Left Plant ---
static const SpriteRect p_f2_rects[] = {
    { 10, 0, 12, 10, YELLOW }, // Helmet
    { 8, 10, 16, 12, RED },    // Jersey
    { 4, 10, 4, 10, PINK },    // Left Arm
    { 24, 10, 4, 10, PINK },   // Right Arm
    { 8, 22, 6, 10, WHITE },   // Left Leg
    { 18, 22, 6, 10, WHITE },  // Right Leg
    { 8, 32, 6, 2, BLACK },    // Left Shoe
    { 18, 32, 6, 2, BLACK }    // Right Shoe
};

// --- Frame 3: Right Leg Up / Arms Swing ---
static const SpriteRect p_f3_rects[] = {
    { 10, 1, 12, 10, YELLOW }, // Helmet (Bob)
    { 8, 11, 16, 12, RED },    // Jersey
    { 4, 14, 4, 10, PINK },    // Left Arm (Down)
    { 24, 8, 4, 10, PINK },    // Right Arm (Up)
    { 8, 23, 6, 10, WHITE },   // Left Leg (Plant)
    { 18, 20, 6, 8, WHITE },   // Right Leg (Lifted)
    { 8, 33, 6, 2, BLACK },    // Left Shoe
    { 18, 28, 6, 2, BLACK }    // Right Shoe (Lifted)
};

static const Sprite buck_e_frames[4] = {
    { 8, p_f0_rects },
    { 8, p_f1_rects },
    { 8, p_f2_rects },
    { 8, p_f3_rects }
};

/* ==========================================
   DEFENDER SPRITES (Front View - Running Down)
   Colors: Grey Helmet, Blue Jersey, Grey Pants
   ========================================== */

// --- Frame 0: Neutral ---
static const SpriteRect d_f0_rects[] = {
    { 10, 0, 12, 10, GREY },   // Helmet
    { 12, 4, 8, 4, BLACK },    // Face Mask
    { 8, 10, 16, 12, BLUE },   // Jersey
    { 4, 10, 4, 10, PINK },    // Left Arm
    { 24, 10, 4, 10, PINK },   // Right Arm
    { 8, 22, 6, 10, GREY },    // Left Leg
    { 18, 22, 6, 10, GREY },   // Right Leg
    { 8, 32, 6, 2, BLACK },    // Left Shoe
    { 18, 32, 6, 2, BLACK }    // Right Shoe
};

// --- Frame 1: Left Leg Forward ---
static const SpriteRect d_f1_rects[] = {
    { 10, 1, 12, 10, GREY },   // Helmet
    { 12, 5, 8, 4, BLACK },    // Face Mask
    { 8, 11, 16, 12, BLUE },   // Jersey
    { 4, 14, 4, 10, PINK },    // Left Arm (Back)
    { 24, 8, 4, 10, PINK },    // Right Arm (Forward)
    { 8, 20, 6, 8, GREY },     // Left Leg (Up)
    { 18, 23, 6, 10, GREY },   // Right Leg (Down)
    { 8, 28, 6, 2, BLACK },    // Left Shoe
    { 18, 33, 6, 2, BLACK }    // Right Shoe
};

// --- Frame 2: Neutral ---
static const SpriteRect d_f2_rects[] = {
    { 10, 0, 12, 10, GREY },   // Helmet
    { 12, 4, 8, 4, BLACK },    // Face Mask
    { 8, 10, 16, 12, BLUE },   // Jersey
    { 4, 10, 4, 10, PINK },    // Left Arm
    { 24, 10, 4, 10, PINK },   // Right Arm
    { 8, 22, 6, 10, GREY },    // Left Leg
    { 18, 22, 6, 10, GREY },   // Right Leg
    { 8, 32, 6, 2, BLACK },    // Left Shoe
    { 18, 32, 6, 2, BLACK }    // Right Shoe
};

// --- Frame 3: Right Leg Forward ---
static const SpriteRect d_f3_rects[] = {
    { 10, 1, 12, 10, GREY },   // Helmet
    { 12, 5, 8, 4, BLACK },    // Face Mask
    { 8, 11, 16, 12, BLUE },   // Jersey
    { 4, 8, 4, 10, PINK },     // Left Arm (Forward)
    { 24, 14, 4, 10, PINK },   // Right Arm (Back)
    { 8, 23, 6, 10, GREY },    // Left Leg (Down)
    { 18, 20, 6, 8, GREY },    // Right Leg (Up)
    { 8, 33, 6, 2, BLACK },    // Left Shoe
    { 18, 28, 6, 2, BLACK }    // Right Shoe
};

static const Sprite defender_frames[4] = {
    { 9, d_f0_rects },
    { 9, d_f1_rects },
    { 9, d_f2_rects },
    { 9, d_f3_rects }
};

#endif // SPRITES_H
