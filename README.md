# CHIP8
A CHIP-8 emulator and debugger created in C++, rendered with SDL3.

![Pong](screenshots/PONG.png "Pong")
*Pong*

![Tetris](screenshots/TETRIS.png "Tetris")
*Tetris*

## Building
```
$ make
```

## Running
```
$ CHIP8.exe PATH/TO/FILE
```

## Controls
| Key | Action |
| --- | --- |
| `1234` / `qwer` / `asdf` / `zxcv` | CHIP-8 hex keypad (`1`-`4`, `Q`-`R`, `A`-`F`, `Z`-`V`) |
| `Space` | Pause / resume |
| `F5` | Reset (reloads the ROM) |
| `F7` | Single-step one opcode (while paused) |