#include <iostream>
#include <cmath>
#include <SDL3/SDL.h>

#include "chip8.hpp"

#define WIDTH 640
#define HEIGHT 320

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: ./CHIP8.exe \"PATH\\TO\\FILENAME\"" << std::endl;
        return 1;
    }
    chip8 CHIP8 = chip8(argv[1]);

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        std::cerr << "Could not init SDL: " << SDL_GetError() << std::endl;
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow("CHIP8", WIDTH, HEIGHT, SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) {
        std::cerr << "Could not create window: " << SDL_GetError() << std::endl;
        return 1;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        std::cerr << "Could not create renderer: " << SDL_GetError() << std::endl;
        return 1;
    }
    SDL_SetRenderLogicalPresentation(renderer, WIDTH, HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    SDL_Event windowEvent;
    bool running = true;
    while (running) {
        const Uint64 start = SDL_GetPerformanceCounter();

        if (SDL_PollEvent(&windowEvent)) {
            switch (windowEvent.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    continue;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    switch (windowEvent.key.key) {
                        case SDLK_1: CHIP8.keypad[0x1] = true; break;
                        case SDLK_2: CHIP8.keypad[0x2] = true; break;
                        case SDLK_3: CHIP8.keypad[0x3] = true; break;
                        case SDLK_4: CHIP8.keypad[0xC] = true; break;

                        case SDLK_Q: CHIP8.keypad[0x4] = true; break;
                        case SDLK_W: CHIP8.keypad[0x5] = true; break;
                        case SDLK_E: CHIP8.keypad[0x6] = true; break;
                        case SDLK_R: CHIP8.keypad[0xD] = true; break;

                        case SDLK_A: CHIP8.keypad[0x7] = true; break;
                        case SDLK_S: CHIP8.keypad[0x8] = true; break;
                        case SDLK_D: CHIP8.keypad[0x9] = true; break;
                        case SDLK_F: CHIP8.keypad[0xE] = true; break;

                        case SDLK_Z: CHIP8.keypad[0xA] = true; break;
                        case SDLK_X: CHIP8.keypad[0x0] = true; break;
                        case SDLK_C: CHIP8.keypad[0xB] = true; break;
                        case SDLK_V: CHIP8.keypad[0xF] = true; break;
                    }
                    break;
                case SDL_EVENT_KEY_UP:
                    switch (windowEvent.key.key) {
                        case SDLK_1: CHIP8.keypad[0x1] = false; break;
                        case SDLK_2: CHIP8.keypad[0x2] = false; break;
                        case SDLK_3: CHIP8.keypad[0x3] = false; break;
                        case SDLK_4: CHIP8.keypad[0xC] = false; break;

                        case SDLK_Q: CHIP8.keypad[0x4] = false; break;
                        case SDLK_W: CHIP8.keypad[0x5] = false; break;
                        case SDLK_E: CHIP8.keypad[0x6] = false; break;
                        case SDLK_R: CHIP8.keypad[0xD] = false; break;

                        case SDLK_A: CHIP8.keypad[0x7] = false; break;
                        case SDLK_S: CHIP8.keypad[0x8] = false; break;
                        case SDLK_D: CHIP8.keypad[0x9] = false; break;
                        case SDLK_F: CHIP8.keypad[0xE] = false; break;

                        case SDLK_Z: CHIP8.keypad[0xA] = false; break;
                        case SDLK_X: CHIP8.keypad[0x0] = false; break;
                        case SDLK_C: CHIP8.keypad[0xB] = false; break;
                        case SDLK_V: CHIP8.keypad[0xF] = false; break;
                    }
                    break;
            }
        }

        CHIP8.run_cycle();

        SDL_RenderClear(renderer);
        SDL_FRect square = {.x = 0, .y = 0, .w = 10, .h = 10};
        for (uint8_t i = 0; i < 32; ++i) {
            for (uint8_t j = 0; j < 64; ++j) {
                square.x = j * 10;
                square.y = i * 10;
                if (CHIP8.display[i][j]) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 1);
                } else {
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 1);
                }   
                SDL_RenderFillRect(renderer, &square);

                // Draw pixel outline
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 1);
                SDL_RenderRect(renderer, &square);
            }
        }
        SDL_RenderPresent(renderer);
        
        // Attempt to cap at 60 FPS
        const Uint64 end = SDL_GetPerformanceCounter();
        const double elapsedMS = (end - start) / SDL_GetPerformanceFrequency() * 1000;
        SDL_Delay(std::max(floor(16.666f - elapsedMS), 0.0));
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}