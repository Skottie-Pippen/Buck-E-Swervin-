// ------------------------------
//        Key Definitions
// ------------------------------
#define KEY0         0b0001
#define KEY1         0b0010
#define KEY2         0b0100
#define KEY3         0b1000

#define FASTER       1
#define SLOWER       2

#define ABS(x)       (((x) > 0) ? (x) : -(x))

// ------------------------------
//      Game Constants
//         (320x240)
// ------------------------------
#define SCREEN_WIDTH       320
#define SCREEN_HEIGHT      240
#define HUD_HEIGHT         36

// ---- Lane Configuration ----
#define NUM_LANES          5
#define LANE_WIDTH         60
#define GAME_X_OFFSET      10

// ---- Player & Obstacles ----
#define PLAYER_SIZE        32
#define OBSTACLE_HEIGHT    32
#define OBSTACLE_WIDTH     32

// ---- Initial Positions ----
#define PLAYER_START_LANE  2
#define PLAYER_START_Y     (SCREEN_HEIGHT - PLAYER_SIZE - 10)

// ---- Obstacle Generation ----
#define MAX_OBSTACLES      15
#define MAX_LIVES          3

// ---- Difficulty Speeds ----
#define SPEED_EASY         2
#define SPEED_MEDIUM       3
#define SPEED_HARD         5
#define SPEED_EXTREME      7

// ------------------------------
//     RGB565 Color Definitions
//           (16-bit)
// ------------------------------
#define WHITE              0xFFFF
#define YELLOW             0xFFE0
#define RED                0xF800
#define GREEN              0x07E0
#define BLUE               0x001F
#define CYAN               0x07FF
#define MAGENTA            0xF81F
#define GREY               0xAD55
#define PINK               0xF97F
#define ORANGE             0xFD20
#define BLACK              0x0000

#define FIELD_COLOR        0x0520
#define HUD_BG_COLOR       0x3186
#define OVERLAY_BG_COLOR   0x2104
#define BUCK_E_COLOR       YELLOW
#define OBSTACLE_COLOR     RED
