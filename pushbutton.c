# Buck-E Swervin' - FPGA Arcade Football Game

## How to Run
To run the game on the DE1-SoC board, follow these steps:

1.  **Load Board Drivers**:
    Navigate to the drivers directory and run the load script:
    ```bash
    cd /home/root/Linux_Libraries/drivers
    ./load_drivers
    ```

2.  **Unload Default Video Driver**:
    The default video driver must be removed before loading the custom game driver:
    ```bash
    rmmod video.ko
    ```

3.  **Build the Project**:
    Navigate to the code folder and compile the game and modules:
    ```bash
    make
    ```

4.  **Load Custom Video Driver**:
    Insert the custom video kernel module:
    ```bash
    insmod video.ko
    ```

5.  **Run the Game**:
    Start the game executable:
    ```bash
    ./bucke_swervin
    ```

    *Note: To unload the custom video driver later, run `rmmod video.ko`.*

## Overview
**Buck-E Swervin'** is a football-themed arcade game developed for the **Intel DE1-SoC** development board. The player controls "Buck-E", a football player who must swerve through lanes to avoid oncoming obstacles (defenders) while running down the field. The game leverages the hardware capabilities of the DE1-SoC, utilizing the FPGA for video output and input handling through custom Linux kernel modules.

## Game Rules & How to Play

### Objective
The goal is to survive as long as possible by dodging obstacles. Your score increases for every obstacle you successfully avoid. The game gets progressively harder as you increase the difficulty speed.

### Controls
- **KEY0**: Move **RIGHT** / **Start Game** / **Restart Game**
- **KEY1**: Move **LEFT**
- **SW[1:0]**: Adjust **Difficulty Speed** (Real-time)
  - `00`: Easy
  - `01`: Medium
  - `10`: Hard
  - `11`: Extreme

### Gameplay Mechanics
1.  **Start**: The game begins in a setup state. Press **KEY0** to start the match.
2.  **Play**:
    - You control the yellow character (Buck-E) at the bottom of the screen.
    - Red obstacles fall from the top of the screen in random lanes.
    - You have **3 Lives** (represented by hearts).
    - Hitting an obstacle costs one life.
    - The background scrolls to simulate running down a football field, complete with yard lines and numbers.
3.  **Game Over**: When all 3 lives are lost, the game ends. Your final score is displayed. Press **KEY0** to play again.

## Project Objectives
The primary objective of this project was to demonstrate **Hardware-Software Co-design** by building a complete embedded system application on the DE1-SoC board. Key goals included:

1.  **Kernel Module Development**: Writing custom Linux device drivers to interface with the FPGA hardware (Video buffer, Pushbuttons, and Switches).
2.  **Memory Mapped I/O**: Understanding and utilizing memory mapping to communicate between the HPS (Hard Processor System) and the FPGA fabric.
3.  **Real-Time Graphics**: Implementing a double-buffered video driver to ensure smooth, tear-free animation at 60 FPS.
4.  **Game Logic**: Developing a robust C application to manage game states, collision detection, and user input.

## Technical Implementation Details

The project consists of a user-space game application and two kernel-space device drivers.

### 1. Video Driver (`video.c`)
This kernel module manages the VGA display. It exposes a character device `/dev/video` that accepts high-level drawing commands.
- **Double Buffering**: To prevent screen tearing, the driver maintains two frame buffers. It draws to the "back buffer" and swaps it with the "front buffer" only during the Vertical Sync (VSync) interval.
- **Drawing Primitives**: The driver implements efficient algorithms for drawing pixels, lines, boxes, and text directly into the FPGA's pixel memory.
- **Command Interface**: The user application sends string commands (e.g., `box 10,10 20,20 255`) to the driver, which parses and executes them.

### 2. Input Driver (`KEY_SW.c`)
This kernel module handles user input from the board's physical controls.
- **Hardware Access**: It maps the Lightweight HPS-to-FPGA bridge to access the memory-mapped registers for the Pushbuttons and Slider Switches.
- **Interface**: It exposes `/dev/KEY` and `/dev/SW`. Reading from these files returns the current state of the buttons/switches as ASCII strings (e.g., "1\n"), making it easy to parse in the user application.

### 3. Audio Subsystem (`audio.c`)
Provides sound effects through direct hardware access:
- **Hardware Interface**: Memory-maps the audio core at 0xFF203040 via `/dev/mem`.
- **Waveform Generation**: Creates sine waves at various frequencies to produce tones.
- **Sound Effects**:
  - **Whistle** (C5→G4): Played when starting a new game
  - **Collision** (120Hz→90Hz): Low "crunch" when hitting a defender
  - **Score** (C4→E4): Quick ascending beep when dodging successfully
  - **Game Over** (A4→F4→D5→C4): Descending melody when all lives lost
- **FIFO Management**: Non-blocking audio buffer writes with space checking.

### 4. Game Application (`main.c` & `pushbutton.c`)
The main C program ties everything together:
- **State Machine**: Manages transitions between `STATE_SETUP`, `STATE_PLAY`, and `STATE_GAMEOVER`.
- **Rendering Loop**:
    1.  Waits for VSync (via the `sync` command to the video driver).
    2.  Reads inputs from `/dev/KEY` and `/dev/SW`.
    3.  Updates game logic (player position, obstacle movement, collision checks).
    4.  Sends drawing commands to `/dev/video` to render the frame.
- **Scrolling Field**: Implements a scrolling background effect with yard lines that update dynamically to create the illusion of forward movement.

## File Structure
- **`main.c`**: Entry point. Contains the main game loop, rendering logic, and state management.
- **`video.c`**: Linux kernel module for the VGA video driver.
- **`KEY_SW.c`**: Linux kernel module for the Pushbutton and Switch drivers.
- **`pushbutton.c`**: Helper logic for processing input and game difficulty.
- **`audio.c`**: Audio subsystem for sound effects using direct hardware access.
- **`audio.h`**: Audio API definitions and sound effect enumeration.
- **`defines.h`**: Shared constants for screen dimensions, colors, and game parameters.
- **`sprites.h`**: Character sprite definitions as collections of colored rectangles.
- **`address_map_arm.h`**: Hardware address definitions for the DE1-SoC.
- **`Makefile`**: Build script for compiling the kernel modules and the game application.
