# Space Defender C++

2D space shooter written in C++ with [Raylib](https://www.raylib.com/). Every graphic (ship, enemies, lasers, explosions and the scrolling starfield) is drawn with geometric primitives, so the game needs no external assets.

## Play without compiling

1. Open [`jogo_espacial.exe`](jogo_espacial.exe) and click **Download raw file**.
2. Double-click the downloaded file. It runs on its own; nothing else needs to be installed.

Windows may show "Windows protected your PC" because the file is not digitally signed. Click **More info → Run anyway**.

## Controls

| Key | Action |
|---|---|
| Left/Right arrows or A/D | Move the ship |
| Space | Shoot |
| Enter | Start (title screen) |
| R | Restart (Game Over screen) |
| Esc | Quit |

## Gameplay

- Enemies spawn at random positions at the top and fly down toward you.
- Each enemy destroyed is worth 10 points and explodes into particles.
- Enemies get faster and appear more often as your score goes up.
- You have 3 lives. You lose one when an enemy hits your ship or gets past the bottom of the screen.

## Build from source (Windows, MSYS2)

With [MSYS2](https://www.msys2.org) installed in `C:\msys64`, double-click `compilar.bat`. It installs Raylib with `pacman` if it is missing, builds `jogo_espacial.exe` and launches it.

To build by hand, run these in the **MSYS2 UCRT64** terminal:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-raylib
```

```bash
g++ main.cpp -o jogo_espacial.exe -std=c++17 -O2 -static -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows
```
