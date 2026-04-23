#ifndef AUDIO_H_
#define AUDIO_H_

#include <stdint.h>

#define PI 3.14159265
#define PI2 6.28318531
#define SAMPLING_RATE 8000
#define MAX_VOLUME 0x7fffffff

// Audio Core Registers (word offsets)
#define FIFOSPACE 1
#define LDATA 2
#define RDATA 3

// Sound effect types
typedef enum {
    SOUND_NONE = 0,     // No sound
    SOUND_WHISTLE,      // Game start
    SOUND_COLLISION,    // Hit defender
    SOUND_SCORE,        // Dodge success
    SOUND_GAMEOVER      // All lives lost
} SoundEffect;

/**
 * Initialize the audio subsystem
 * Returns: 1 on success, 0 on failure
 */
int audio_init(void);

/**
 * Play a sound effect
 * Parameter effect: Which sound effect to play
 */
void audio_play_effect(SoundEffect effect);

/**
 * Write audio samples to the audio core
 * Parameter left: Left channel sample
 * Parameter right: Right channel sample
 * Returns: 1 if written successfully, 0 if FIFO full
 */
int audio_write_samples(int32_t left, int32_t right);

/**
 * Get available space in audio FIFO
 * Returns: Number of words available in FIFO
 */
int audio_get_fifo_space(void);

/**
 * Stop all audio playback
 */
void audio_stop(void);

/**
 * Close audio subsystem
 */
void audio_close(void);

#endif /*AUDIO_H_*/
