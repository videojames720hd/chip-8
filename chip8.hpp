#include <stdint.h>

class chip8 {
public:
    chip8(std::string fileName);
    void run_cycle();

    bool display[32][64];
    bool keypad[16];
    bool isSoundActive();
private:
    uint16_t PC; // Program Counter
    uint16_t I; // Index Register
    uint16_t SP; // Stack Pointer

    uint16_t stack[16];
    uint8_t V[16]; // Registers
    uint8_t mem[4096];
    uint8_t delayTimer;
    uint8_t soundTimer;
};