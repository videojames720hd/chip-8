#include <algorithm>
#include <iostream>
#include <filesystem>
#include <vector>
#include <SDL3/SDL.h>

#include "chip8.hpp"
#include "ui.hpp"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: ./CHIP8.exe \"PATH\\TO\\FILENAME\"" << std::endl;
        return 1;
    }
    chip8 CHIP8 = chip8(argv[1]);
    const std::string romName = std::filesystem::path(argv[1]).filename().string();

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        std::cerr << "Could not init SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    ui::Context uiCtx = ui::makeContext();
    const ui::Layout layout = ui::computeLayout(uiCtx);

    SDL_Window *window = SDL_CreateWindow("CHIP8", layout.windowW, layout.windowH, SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) {
        std::cerr << "Could not create window: " << SDL_GetError() << std::endl;
        return 1;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        std::cerr << "Could not create renderer: " << SDL_GetError() << std::endl;
        return 1;
    }
    SDL_SetRenderLogicalPresentation(renderer, layout.windowW, layout.windowH, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetRenderVSync(renderer, 1);
    uiCtx.renderer = renderer;

    bool paused = false;

    SDL_AudioSpec audioSpec;
    audioSpec.format = SDL_AUDIO_S16;
    audioSpec.channels = 1;
    audioSpec.freq = 44100;
    SDL_AudioStream *audioStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audioSpec, NULL, NULL);
    if (!audioStream) {
        std::cerr << "Could not open audio stream: " << SDL_GetError() << std::endl;
        return 1;
    }
    SDL_ResumeAudioStreamDevice(audioStream);

    float soundPhase = 0.0f;
    constexpr float toneHz = 440.0f;
    constexpr float sampleRate = 44100.0f;
    constexpr int16_t amplitude = 4000;
    std::vector<int16_t> soundSamples;
    soundSamples.reserve((size_t)(0.25 * sampleRate) + 1);

    constexpr double CPU_HZ = 500.0;
    constexpr double CYCLE_MS = 1000.0 / CPU_HZ;
    double cycleAccumulator = 0.0;

    constexpr double TIMER_HZ = 60.0;
    constexpr double TIMER_MS = 1000.0 / TIMER_HZ;
    double timerAccumulator = 0.0;
    Uint64 previousStart = SDL_GetPerformanceCounter();

    SDL_Event windowEvent;
    bool running = true;
    while (running) {
        const Uint64 start = SDL_GetPerformanceCounter();
        const double frameDeltaMS = (start - previousStart) / (double)SDL_GetPerformanceFrequency() * 1000.0;
        previousStart = start;

        while (SDL_PollEvent(&windowEvent)) {
            switch (windowEvent.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    switch (windowEvent.key.key) {
                        case SDLK_1: CHIP8.setKey(0x1, true); break;
                        case SDLK_2: CHIP8.setKey(0x2, true); break;
                        case SDLK_3: CHIP8.setKey(0x3, true); break;
                        case SDLK_4: CHIP8.setKey(0xC, true); break;

                        case SDLK_Q: CHIP8.setKey(0x4, true); break;
                        case SDLK_W: CHIP8.setKey(0x5, true); break;
                        case SDLK_E: CHIP8.setKey(0x6, true); break;
                        case SDLK_R: CHIP8.setKey(0xD, true); break;

                        case SDLK_A: CHIP8.setKey(0x7, true); break;
                        case SDLK_S: CHIP8.setKey(0x8, true); break;
                        case SDLK_D: CHIP8.setKey(0x9, true); break;
                        case SDLK_F: CHIP8.setKey(0xE, true); break;

                        case SDLK_Z: CHIP8.setKey(0xA, true); break;
                        case SDLK_X: CHIP8.setKey(0x0, true); break;
                        case SDLK_C: CHIP8.setKey(0xB, true); break;
                        case SDLK_V: CHIP8.setKey(0xF, true); break;

                        case SDLK_SPACE:
                            paused = !paused;
                            break;
                        case SDLK_F5:
                            if (!windowEvent.key.repeat) {
                                CHIP8 = chip8(argv[1]);
                            }
                            break;
                        case SDLK_F7:
                            if (paused && !windowEvent.key.repeat) {
                                CHIP8.run_cycle();
                            }
                            break;
                    }
                    break;
                case SDL_EVENT_KEY_UP:
                    switch (windowEvent.key.key) {
                        case SDLK_1: CHIP8.setKey(0x1, false); break;
                        case SDLK_2: CHIP8.setKey(0x2, false); break;
                        case SDLK_3: CHIP8.setKey(0x3, false); break;
                        case SDLK_4: CHIP8.setKey(0xC, false); break;

                        case SDLK_Q: CHIP8.setKey(0x4, false); break;
                        case SDLK_W: CHIP8.setKey(0x5, false); break;
                        case SDLK_E: CHIP8.setKey(0x6, false); break;
                        case SDLK_R: CHIP8.setKey(0xD, false); break;

                        case SDLK_A: CHIP8.setKey(0x7, false); break;
                        case SDLK_S: CHIP8.setKey(0x8, false); break;
                        case SDLK_D: CHIP8.setKey(0x9, false); break;
                        case SDLK_F: CHIP8.setKey(0xE, false); break;

                        case SDLK_Z: CHIP8.setKey(0xA, false); break;
                        case SDLK_X: CHIP8.setKey(0x0, false); break;
                        case SDLK_C: CHIP8.setKey(0xB, false); break;
                        case SDLK_V: CHIP8.setKey(0xF, false); break;
                    }
                    break;
            }
        }

        // Clamp so a stall (e.g. window drag) doesn't dump a huge catch-up burst.
        const double clampedDeltaMS = std::min(frameDeltaMS, 250.0);

        if (!paused) {
            auto stepFixed = [&](double &acc, double stepMS, auto &&fn) {
                acc += clampedDeltaMS;
                while (acc >= stepMS) {
                    fn();
                    acc -= stepMS;
                }
            };
            stepFixed(cycleAccumulator, CYCLE_MS, [&] { CHIP8.run_cycle(); });
            // Timers tick on their own real-time-driven 60Hz accumulator, independent of
            // both the CPU rate above and the loop's own pacing below.
            stepFixed(timerAccumulator, TIMER_MS, [&] { CHIP8.tick_timers(); });
        }

        const int samplesThisFrame = std::max(1, (int)(clampedDeltaMS / 1000.0 * sampleRate));
        const bool soundActive = !paused && CHIP8.isSoundActive();
        soundSamples.resize(samplesThisFrame);
        for (int i = 0; i < samplesThisFrame; ++i) {
            soundSamples[i] = soundActive ? (soundPhase < 0.5f ? amplitude : -amplitude) : 0;
            soundPhase += toneHz / sampleRate;
            if (soundPhase >= 1.0f) {
                --soundPhase;
            }
        }
        SDL_PutAudioStreamData(audioStream, soundSamples.data(), samplesThisFrame * (int)sizeof(int16_t));

        SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);
        SDL_RenderClear(renderer);

        ui::drawDisplay(uiCtx, CHIP8, layout.display);
        ui::drawHeader(uiCtx, layout.header, romName, (int)CPU_HZ, paused);
        ui::drawCpuPanel(uiCtx, CHIP8, layout.cpu);
        ui::drawStackPanel(uiCtx, CHIP8, layout.stack);
        ui::drawKeypadPanel(uiCtx, CHIP8, layout.keypad);
        ui::drawRegistersPanel(uiCtx, CHIP8, layout.registers);
        ui::drawDisasmPanel(uiCtx, CHIP8, layout.disasm);
        ui::drawMemoryPanel(uiCtx, CHIP8, layout.memory);
        ui::drawFooter(uiCtx, layout.footer);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyAudioStream(audioStream);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}