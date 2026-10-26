#pragma once

#include <SDL3/SDL.h>
#include <string>

#include "chip8.hpp"

namespace ui {

struct Color {
    Uint8 r, g, b, a = 255;
};

inline constexpr Color BRIGHT{242, 242, 242};
inline constexpr Color DIM{120, 120, 120};
inline constexpr Color BG{10, 10, 10};

struct Context {
    SDL_Renderer *renderer;
    float charW;
    float lineH;
};

struct Layout {
    int windowW, windowH;
    SDL_FRect header;
    SDL_FRect display;
    SDL_FRect cpu;
    SDL_FRect stack;
    SDL_FRect keypad;
    SDL_FRect registers;
    SDL_FRect disasm;
    SDL_FRect memory;
    SDL_FRect footer;
};

Context makeContext();

Layout computeLayout(const Context &ctx);

void drawText(const Context &ctx, float x, float y, const std::string &text, Color color);
void drawBox(SDL_Renderer *renderer, const SDL_FRect &rect, Color color);

void drawDisplay(const Context &ctx, chip8 &c, const SDL_FRect &box);
void drawHeader(const Context &ctx, const SDL_FRect &rect, const std::string &romName, int hz, bool paused);
void drawCpuPanel(const Context &ctx, chip8 &c, const SDL_FRect &rect);
void drawStackPanel(const Context &ctx, chip8 &c, const SDL_FRect &rect);
void drawKeypadPanel(const Context &ctx, chip8 &c, const SDL_FRect &rect);
void drawRegistersPanel(const Context &ctx, chip8 &c, const SDL_FRect &rect);
void drawDisasmPanel(const Context &ctx, chip8 &c, const SDL_FRect &rect);
void drawMemoryPanel(const Context &ctx, chip8 &c, const SDL_FRect &rect);
void drawFooter(const Context &ctx, const SDL_FRect &rect);

} // namespace ui
