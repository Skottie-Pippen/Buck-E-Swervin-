#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <math.h>
#include <pthread.h>
#include "audio.h"
#include "address_map_arm.h"

static int fd = -1;
static void *lw_bridge_base = NULL;
static volatile int *audio_base = NULL;

// Threading support
static pthread_t audio_thread;
static pthread_mutex_t audio_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t audio_cond = PTHREAD_COND_INITIALIZER;
static volatile int audio_thread_running = 0;
static volatile SoundEffect pending_sound = SOUND_NONE;
static volatile int shutdown_requested = 0;

// Tone frequencies (in Hz) for sound effects
#define NOTE_C4  262
#define NOTE_E4  330
#define NOTE_G4  392
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_A4  440
#define NOTE_F4  349

// Forward declarations
static void* audio_thread_worker(void* arg);
static void generate_tone(int frequency, int duration_ms, double volume);
static void play_whistle(void);
static void play_collision(void);
static void play_score(void);
static void play_gameover(void);

/**
 * Open /dev/mem and map audio hardware
 */
int audio_init(void) {
    // Open /dev/mem for physical memory access
    if ((fd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1) {
        fprintf(stderr, "ERROR: could not open /dev/mem for audio\n");
        return 0;
    }

    // Map the lightweight bridge
    lw_bridge_base = mmap(NULL, LW_BRIDGE_SPAN, (PROT_READ | PROT_WRITE), 
                          MAP_SHARED, fd, LW_BRIDGE_BASE);
    if (lw_bridge_base == MAP_FAILED) {
        fprintf(stderr, "ERROR: mmap() failed for audio\n");
        close(fd);
        fd = -1;
        return 0;
    }

    // Get pointer to audio core registers
    audio_base = (volatile int *)((char *)lw_bridge_base + AUDIO_BASE);
    
    // Clear audio FIFOs
    audio_stop();
    
    // Start audio thread
    shutdown_requested = 0;
    pending_sound = SOUND_NONE;
    
    if (pthread_create(&audio_thread, NULL, audio_thread_worker, NULL) != 0) {
        fprintf(stderr, "ERROR: failed to create audio thread\n");
        munmap(lw_bridge_base, LW_BRIDGE_SPAN);
        close(fd);
        fd = -1;
        lw_bridge_base = NULL;
        audio_base = NULL;
        return 0;
    }
    
    audio_thread_running = 1;
    
    return 1;
}

/**
 * Get available space in audio FIFO
 */
int audio_get_fifo_space(void) {
    if (audio_base == NULL) return 0;
    return (*(audio_base + FIFOSPACE) & 0xFF000000) >> 24;  // WSRC field
}

/**
 * Write stereo samples to audio core
 */
int audio_write_samples(int32_t left, int32_t right) {
    if (audio_base == NULL) return 0;
    
    // Check if space available
    if (audio_get_fifo_space() < 2) return 0;
    
    // Write to left and right channels
    *(audio_base + LDATA) = left;
    *(audio_base + RDATA) = right;
    
    return 1;
}

/**
 * Generate a simple tone waveform
 */
static void generate_tone(int frequency, int duration_ms, double volume) {
    if (audio_base == NULL) return;
    
    int num_samples = (SAMPLING_RATE * duration_ms) / 1000;
    double phase = 0.0;
    double phase_increment = (PI2 * frequency) / SAMPLING_RATE;
    
    for (int i = 0; i < num_samples; i++) {
        // Generate sine wave
        int32_t sample = (int32_t)(sin(phase) * MAX_VOLUME * volume);
        
        // Wait for FIFO space and write
        while (audio_get_fifo_space() < 2) {
            usleep(100);
        }
        audio_write_samples(sample, sample);
        
        phase += phase_increment;
        if (phase >= PI2) phase -= PI2;
    }
}

/**
 * Play a whistle sound (game start)
 */
static void play_whistle(void) {
    // Two-tone whistle: high then low
    generate_tone(NOTE_C5, 100, 0.3);
    generate_tone(NOTE_G4, 150, 0.3);
}

/**
 * Play collision sound (crunch/hit)
 */
static void play_collision(void) {
    // Low, harsh noise-like tone
    generate_tone(120, 80, 0.4);
    generate_tone(90, 80, 0.35);
}

/**
 * Play score sound (success beep)
 */
static void play_score(void) {
    // Quick ascending beep
    generate_tone(NOTE_C4, 50, 0.25);
    generate_tone(NOTE_E4, 50, 0.25);
}

/**
 * Play game over sound (descending tones)
 */
static void play_gameover(void) {
    // Sad descending sequence
    generate_tone(NOTE_A4, 200, 0.3);
    generate_tone(NOTE_F4, 200, 0.3);
    generate_tone(NOTE_D5, 200, 0.3);
    generate_tone(NOTE_C4, 400, 0.3);
}

/**
 * Audio thread worker function
 */
static void* audio_thread_worker(void* arg) {
    (void)arg; // Unused parameter
    
    while (1) {
        pthread_mutex_lock(&audio_mutex);
        
        // Wait for a sound to play or shutdown signal
        while (pending_sound == SOUND_NONE && !shutdown_requested) {
            pthread_cond_wait(&audio_cond, &audio_mutex);
        }
        
        // Check if we should exit
        if (shutdown_requested) {
            pthread_mutex_unlock(&audio_mutex);
            break;
        }
        
        // Get the sound to play and clear it
        SoundEffect sound = pending_sound;
        pending_sound = SOUND_NONE;
        
        pthread_mutex_unlock(&audio_mutex);
        
        // Play the sound (outside the mutex lock)
        if (audio_base != NULL) {
            switch (sound) {
                case SOUND_WHISTLE:
                    play_whistle();
                    break;
                case SOUND_COLLISION:
                    play_collision();
                    break;
                case SOUND_SCORE:
                    play_score();
                    break;
                case SOUND_GAMEOVER:
                    play_gameover();
                    break;
                case SOUND_NONE:
                    break;
            }
        }
    }
    
    return NULL;
}

/**
 * Play the requested sound effect (non-blocking)
 */
void audio_play_effect(SoundEffect effect) {
    if (audio_base == NULL || !audio_thread_running) return;
    if (effect == SOUND_NONE) return;
    
    pthread_mutex_lock(&audio_mutex);
    
    // Only queue if no sound is currently pending
    // This prevents audio queue backup
    if (pending_sound == SOUND_NONE) {
        pending_sound = effect;
        pthread_cond_signal(&audio_cond);
    }
    
    pthread_mutex_unlock(&audio_mutex);
}

/**
 * Clear audio FIFOs
 */
void audio_stop(void) {
    if (audio_base == NULL) return;
    
    // Clear both FIFOs by reading available space
    int space = audio_get_fifo_space();
    (void)space; // Suppress unused warning
}

/**
 * Clean up audio resources
 */
void audio_close(void) {
    // Signal audio thread to shut down
    if (audio_thread_running) {
        pthread_mutex_lock(&audio_mutex);
        shutdown_requested = 1;
        pthread_cond_signal(&audio_cond);
        pthread_mutex_unlock(&audio_mutex);
        
        // Wait for thread to finish
        pthread_join(audio_thread, NULL);
        audio_thread_running = 0;
    }
    
    audio_stop();
    
    if (lw_bridge_base != NULL && lw_bridge_base != MAP_FAILED) {
        munmap(lw_bridge_base, LW_BRIDGE_SPAN);
        lw_bridge_base = NULL;
    }
    
    if (fd != -1) {
        close(fd);
        fd = -1;
    }
    
    audio_base = NULL;
}
