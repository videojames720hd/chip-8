#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <time.h> 

#include "chip8.hpp"

#define X (opcode & 0x0F00) >> 8
#define Y (opcode & 0x00F0) >> 4
#define F 0x000F
#define NNN (opcode & 0x0FFF)
#define NN (opcode & 0x00FF)
#define N (opcode & 0x000F)

chip8::chip8(std::string fileName)
{
    const uint8_t font[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0,	// 0
        0x20, 0x60, 0x20, 0x20, 0x70,	// 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0,	// 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0,	// 3
        0x90, 0x90, 0xF0, 0x10, 0x10,	// 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0,	// 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0,	// 6
        0xF0, 0x10, 0x20, 0x40, 0x40,	// 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0,	// 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0,	// 9
        0xF0, 0x90, 0xF0, 0x90, 0x90,	// A
        0xE0, 0x90, 0xE0, 0x90, 0xE0,	// B
        0xF0, 0x80, 0x80, 0x80, 0xF0,	// C
        0xE0, 0x90, 0x90, 0x90, 0xE0,	// D
        0xF0, 0x80, 0xF0, 0x80, 0xF0,	// E
        0xF0, 0x80, 0xF0, 0x80, 0x80    // F
    };

    // Load font
    memcpy(&mem[0], font, sizeof(font));

    // Load ROM
    FILE *rom = fopen(fileName.c_str(), "rb");
    if (!rom) {
        std::cerr << "Not a valid file" << std::endl;
        exit(1);
    }
    fseek(rom, 0, SEEK_END);
    const uint32_t size = ftell(rom);
    //std::cout << "size: " << size << std::endl;
    rewind(rom);
    fread(&mem[0x200], size, 1, rom);
    fclose(rom);

    memset(this->display, 0, sizeof(this->display));
    memset(this->stack, 0, sizeof(this->stack));
    memset(this->V, 0, sizeof(this->V));
    PC = 0x200;
    SP = 0;

    srand(time(NULL));
}

void chip8::run_cycle()
{
    // Decrement timers
    this->delayTimer = std::max(this->delayTimer - 1, 0);
    this->soundTimer = std::max(this->soundTimer - 1, 0);

    uint16_t opcode = this->mem[this->PC] << 8 | this->mem[this->PC + 1];
    this->PC += 2;
    //std::cout << std::hex << opcode << std::endl;

    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode & 0x00FF) {
                case 0x00E0: // 00E0 - CLS
                    memset(this->display, 0, sizeof(this->display));
                    break;
                case 0x00EE: // 00EE - RET
                    this->PC = this->stack[this->SP];
                    this->stack[this->SP] = 0;
                    this->SP = std::max(this->SP - 1, 0);
                    break;
            }
            break;
        case 0x1000: // 1nnn - JP addr
            this->PC = NNN;
            break;
        case 0x2000: // 2nnn - CALL addr
            if (this->stack[this->SP]) {
                ++this->SP;
            }
            this->stack[this->SP] = this->PC;
            this->PC = NNN; 
            break;
        case 0x3000: // 3xnn - SE Vx, byte
            if (this->V[X] == NN) {
                this->PC += 2;
            }
            break;
        case 0x4000: // 4xnn - SNE Vx, byte
            if (this->V[X] != NN) {
                this->PC += 2;
            }
            break;
        case 0x5000: // 5xy0 - SE Vx, Vy
            if (this->V[X] == this->V[Y]) {
                this->PC += 2;
            }
            break;
        case 0x6000: // 6xnn - LD Vx, byte
            this->V[X] = NN;
            break;
        case 0x7000: // 7xnn - ADD Vx, byte
            this->V[X] += NN;
            break;
        case 0x8000:
            switch (opcode & 0x000F) {
                case 0x0000: // 8xy0 - LD Vx, Vy
                    this->V[X] = this->V[Y];
                    break;
                case 0x0001: // 8xy1 - OR Vx, Vy
                    this->V[X] |= this->V[Y];
                    break;
                case 0x0002: // 8xy2 - AND Vx, Vy
                    this->V[X] &= this->V[Y];
                    break;
                case 0x0003: // 8xy3 - XOR Vx, Vy
                    this->V[X] ^= this->V[Y];
                    break;
                case 0x0004: // 8xy4 - ADD Vx, Vy
                    this->V[X] += this->V[Y];
                    this->V[F] = this->V[X] < this->V[Y];
                    break;
                case 0x0005: {// 8xy5 - SUB Vx, Vy
                    const uint8_t no_borrow = this->V[X] >= this->V[Y];
                    this->V[X] -= this->V[Y];
                    this->V[F] = no_borrow;
                    break;
                }
                case 0x0006: // 8xy6 - SHR Vx {, Vy}
                    this->V[F] = this->V[X] & 1;
                    this->V[X] >>= 1;
                    break;
                case 0x0007: // 8xy7 - SUBN Vx, Vy
                    this->V[F] = this->V[Y] < this->V[X] ? 0 : 1;
                    this->V[X] =  this->V[Y] - this->V[X];
                    break;
                case 0x000E: // 8xyE - SHL Vx {, Vy}
                    this->V[F] = this->V[X] < 128 ? 0 : 1;
                    this->V[X] <<= 1;
                    break;
            }
            break;
        case 0x9000: // 9xy0 - SNE Vx, Vy
            if (this->V[X] != this->V[Y]) {
                this->PC += 2;
            }
            break;
        case 0xA000: // Annn - LD I, addr
            this->I = NNN;
            break;
        case 0xB000: // Bnnn - JP V0, addr
            this->PC = this->V[0] + NNN;
            break;
        case 0xC000: // Cxnn - RND Vx, byte
            this->V[X] = (rand() % 256) & NN;
            break;
        case 0xD000: { // Dxyn - DRW Vx, Vy, nibble
            this->V[F] = 0;

            const uint8_t x_coord = this->V[X] % 64; 
            const uint8_t y_coord = this->V[Y] % 32;
            for (uint8_t i = 0; i < N; ++i) {
                if (y_coord + i == 32) {
                    break;
                }
                const uint8_t sprite_data = this->mem[this->I + i];
                for (uint8_t j = 0; j < 8; ++j) {
                    if (x_coord + j == 64) {
                        break;
                    }
                    const bool sprite_bit = (sprite_data >> (8 - j - 1)) & 1;
                    if (this->display[y_coord + i][x_coord + j] && sprite_bit) {
                        this->V[F] = 1;
                    }
                    this->display[y_coord + i][x_coord + j] ^= sprite_bit;
                }
            }
            break;
        }
        case 0xE000:
            switch (opcode & 0x00FF) {
                case 0x009E: // Ex9E - SKP Vx
                    if (this->keypad[this->V[X]]) {
                        this->PC += 2;
                    }
                    break;
                case 0x00A1: // ExA1 - SKNP Vx
                    if (!this->keypad[this->V[X]]) {
                        this->PC += 2;
                    }
                    break;
            }
            break;
        case 0xF000:
            switch (opcode & 0x00FF) {
                case 0x0007: // Fx07 - LD Vx, DT
                    this->V[X] = this->delayTimer;
                    break;
                case 0x000A: { // Fx0A - LD Vx, K
                    bool pressed = false;
                    for (uint8_t i = 0; i < 16; ++i) {
                        if (this->keypad[i]) {
                            this->V[X] = i;
                            pressed = true;
                            break;
                        }
                    }
                    if (pressed) {
                        this->PC -=2;
                    }
                    break;
                }
                case 0x0015: // Fx15 - LD DT, Vx
                    this->delayTimer = this->V[X];
                    break;
                case 0x0018: // Fx18 - LD ST, Vx
                    this->soundTimer = this->V[X];
                    break;
                case 0x001E: // Fx1E - ADD I, Vx
                    this->I += this->V[X];
                    break;
                case 0x0029: // Fx29 - LD F, Vx
                    this->I = this->V[X] * 5;
                    break;
                case 0x0033: // Fx33 - LD B, Vx
                    this->mem[this->I] = this->V[X] / 100;
                    this->mem[this->I + 1] = (this->V[X] / 10) % 10;
                    this->mem[this->I + 2] = this->V[X] % 10;
                    break;
                case 0x0055: // Fx55 - LD [I], Vx
                    for (uint8_t i = 0; i <= X; ++i) {
                        this->mem[this->I + i] = this->V[i];
                    }
                    break;
                case 0x0065: // Fx65 - LD Vx, [I]
                    for (uint8_t i = 0; i <= X; ++i) {
                        this->V[i] = this->mem[this->I + i];
                    }
                    break;
            }
            break;
        default:
            std::cerr << "Unknown opcode: " << std::hex << opcode << std::endl;
            exit(1);
    }
}