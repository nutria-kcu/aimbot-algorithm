# External Aimbot (AssaultCube)

This project implements an **external memory-based aimbot** for AssaultCube using Windows API and memory offsets. The aimbot automatically aims at the closest visible enemy when the user is holding the left mouse button. It uses world-to-screen projection and smooth aiming based on target distance.

---

## Features

- External memory access via `ReadProcessMemory` / `WriteProcessMemory`
- Smooth aiming using linear interpolation (LERP)
- Automatically targets the closest visible enemy
- Ignores dead enemies and players behind walls
- FOV limit based on pixel distance from screen center
- Aimbot toggled with a boolean flag (`aimbotEnabled`)

---

## File Structure

| File | Description |
|------|-------------|
| `Aimbot.cpp` | Main implementation logic for the aimbot loop and memory handling |
| `AimbotHeader.h` | Header file with structure declarations and function prototypes |
| `constants.h` | Contains memory offsets specific to AssaultCube |

---

## How It Works

1. Reads local player info, view matrix, entity list, and camera angle.
2. Filters valid targets based on:
   - Alive status
   - Visibility via `WorldToScreen`
   - FOV pixel range limit
3. Calculates aim angle using `CalcAngle` and smoothly adjusts aim using `AimAtTarget`.
4. Aimbot runs **only while left mouse button is held** and `aimbotEnabled` is `true`.

---

## How to Use

1. Launch AssaultCube.
2. Attach the program to the game's process and initialize `hProc` and `baseAddress`.
3. Call `Tick()` in your main loop.
4. Hold left mouse button to activate aiming.

---

## Configuration

- Adjust `maxFOVPixels` in `Tick()` to change how far from the crosshair enemies are allowed to be targeted.
- Tweak `priority` formula to add health or team-based weight if desired.
- Replace the auto `percent` calculation with a custom smoothness value if UI input is integrated.

---

## Disclaimer

This code is intended for **educational and research purposes only**. Do not use it in online or competitive games.

---

## Author

Dowon Lee
