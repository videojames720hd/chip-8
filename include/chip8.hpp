#pragma once

#include <stdint.h>
#include <string>

class chip8 {
public:
    chip8(std::string fileName);
    void run_cycle();
    void tick_timers(); // decrements delay/sound timers; call at a fixed 60Hz, independent of CPU rate

    bool isSoundActive() const;

    bool getPixel(uint8_t row, uint8_t col);

    bool getKey(uint8_t key);
    void setKey(uint8_t key, bool pressed);

    uint16_t getPC();
    uint16_t getI();
    uint16_t getSP();
    uint16_t getOpcode();

    uint16_t getStack(uint8_t depth);
    uint8_t getV(uint8_t index);
    uint8_t getDelayTimer();
    uint8_t getSoundTimer();
    uint8_t getMem(uint16_t addr);

private:
    bool display[32][64];
    bool keypad[16];

    uint16_t PC = 0x200; // Program Counter
    uint16_t I = 0; // Index Register
    uint16_t SP = 0; // Stack Pointer
    uint16_t opcode = 0; // Last-fetched opcode

    uint16_t stack[16];
    uint8_t V[16]; // Registers
    uint8_t delayTimer = 0;
    uint8_t soundTimer = 0;

    uint8_t mem[4096];
};

// Decodes a raw opcode into its mnemonic (e.g. 0x6A02 -> "LD VA, 0x02").
// Covers every opcode run_cycle() implements; anything else comes back as "??? nnnn".
std::string disassemble(uint16_t opcode);
