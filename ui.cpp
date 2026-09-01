#include <algorithm>
#include <cstdio>

#include "ui.hpp"

namespace {

constexpr float MARGIN = 12;
constexpr float GAP = 12;
constexpr float PANEL_PAD_X = 9;
constexpr float PANEL_PAD_Y = 5;
constexpr float BORDER = 1;
constexpr float DISPLAY_W = 640;
constexpr float DISPLAY_H = 320;
constexpr float DISPLAY_PAD = 8;
constexpr float KEY_CELL = 44;
constexpr float KEY_GAP = 4;

constexpr float LABEL_GAP = 2;

constexpr float LINE_SPACING = 5;

// DISASM: instruction rows around PC, with PC anchored below a few leading rows.
constexpr int DISASM_ROWS = 9;
constexpr int DISASM_ROWS_BEFORE_PC = 3;
// Widest row the disassembler can produce: "> 0200  D015  DRW V0, V1, 6".
constexpr int DISASM_COLS = 27;

// MEMORY: rows of raw bytes anchored around I.
constexpr int MEM_ROWS = 8;
constexpr int MEM_BYTES = 8;
// "0200" + two spaces + "xx " per byte (no trailing space).
constexpr int MEM_COLS = 6 + MEM_BYTES * 3 - 1;
constexpr int MEM_WINDOW = MEM_ROWS * MEM_BYTES;

struct KeyCell { uint8_t hex; const char *label; };
constexpr KeyCell KEY_LAYOUT[16] = {
    {0x1, "1"}, {0x2, "2"}, {0x3, "3"}, {0xC, "4"},
    {0x4, "Q"}, {0x5, "W"}, {0x6, "E"}, {0xD, "R"},
    {0x7, "A"}, {0x8, "S"}, {0x9, "D"}, {0xE, "F"},
    {0xA, "Z"}, {0x0, "X"}, {0xB, "C"}, {0xF, "V"},
};

float drawPanel(const ui::Context &ctx, const SDL_FRect &rect, const std::string &label)
{
    ui::drawBox(ctx.renderer, rect, ui::DIM);
    ui::drawText(ctx, rect.x + PANEL_PAD_X, rect.y + PANEL_PAD_Y, label, ui::DIM);
    return rect.y + PANEL_PAD_Y + ctx.lineH + LABEL_GAP;
}

// Content height for a panel with one label row plus `rows` value rows.
float panelHeight(const ui::Context &ctx, int rows)
{
    return 2 * PANEL_PAD_Y + ctx.lineH + LABEL_GAP + rows * ctx.lineH + 2 * BORDER;
}

float panelWidth(const ui::Context &ctx, int cols)
{
    return cols * ctx.charW + 2 * PANEL_PAD_X + 2 * BORDER;
}

float textW(const ui::Context &ctx, const std::string &text)
{
    return text.size() * ctx.charW;
}

void fillRect(SDL_Renderer *renderer, const SDL_FRect &rect, ui::Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

void drawHighlightedText(const ui::Context &ctx, const SDL_FRect &hi, float x, float y, const std::string &text)
{
    fillRect(ctx.renderer, hi, ui::BRIGHT);
    ui::drawText(ctx, x, y, text, ui::BG);
}

} // namespace

ui::Context ui::makeContext()
{
    return Context{nullptr, (float)SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE,
                    (float)SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE + LINE_SPACING};
}

ui::Layout ui::computeLayout(const Context &ctx)
{
    Layout L{};

    const float lineH = ctx.lineH;

    const float headerH = lineH + 8;
    const float footerH = lineH;

    const float displayOuterW = DISPLAY_W + 2 * DISPLAY_PAD + 2 * BORDER;
    const float displayOuterH = DISPLAY_H + 2 * DISPLAY_PAD + 2 * BORDER;

    const float rightColW = panelWidth(ctx, 22);

    // CPU: 3 value rows. STACK: 8 rows (16 slots / 2 cols).
    const float cpuH = panelHeight(ctx, 3);
    const float stackH = panelHeight(ctx, 8);
    const float rightColH = cpuH + GAP + stackH;

    const float row2H = std::max(displayOuterH, rightColH);
    const float top = MARGIN + headerH + GAP;

    L.header = {MARGIN, MARGIN, displayOuterW + GAP + rightColW, headerH};
    // Fixed to the actual 64x32 resolution so the border always hugs the real pixel
    // grid; if the right column (CPU/STACK) ends up taller, center the display in
    // that extra vertical space rather than stretching its border past the content.
    L.display = {MARGIN, top + (row2H - displayOuterH) / 2, displayOuterW, displayOuterH};

    const float rightX = MARGIN + displayOuterW + GAP;
    L.cpu = {rightX, top, rightColW, cpuH};
    L.stack = {rightX, top + cpuH + GAP, rightColW, stackH};

    const float bottomTop = top + row2H + GAP;

    const float keypadContentW = 4 * KEY_CELL + 3 * KEY_GAP;
    const float keypadContentH = 4 * KEY_CELL + 3 * KEY_GAP;
    const float keypadLabelH = lineH + 4;
    const float keypadW = keypadContentW + 2 * PANEL_PAD_X + 2 * BORDER;
    const float keypadMinH = keypadContentH + keypadLabelH + PANEL_PAD_Y + 2 * BORDER;

    // REGISTERS: V0-VF as 8 rows of 2 columns.
    const float regW = panelWidth(ctx, 13);
    const float regMinH = panelHeight(ctx, 8);

    // DISASM / MEMORY: sized from their own row counts, like every other panel.
    const float disasmW = panelWidth(ctx, DISASM_COLS);
    const float disasmMinH = panelHeight(ctx, DISASM_ROWS);
    const float memW = panelWidth(ctx, MEM_COLS);
    const float memMinH = panelHeight(ctx, MEM_ROWS);

    const float bottomRowH = std::max({keypadMinH, regMinH, disasmMinH, memMinH});

    const float regX = MARGIN + keypadW + GAP;
    const float disasmX = regX + regW + GAP;
    const float memX = disasmX + disasmW + GAP;

    L.keypad = {MARGIN, bottomTop, keypadW, bottomRowH};
    L.registers = {regX, bottomTop, regW, bottomRowH};
    L.disasm = {disasmX, bottomTop, disasmW, bottomRowH};
    L.memory = {memX, bottomTop, memW, bottomRowH};

    // The bottom row is now the widest row; header/footer span the full content width.
    const float contentW = std::max(displayOuterW + GAP + rightColW, memX + memW - MARGIN);
    L.header.w = contentW;

    const float footerTop = bottomTop + bottomRowH + GAP;
    L.footer = {MARGIN, footerTop, contentW, footerH};

    L.windowW = (int)(2 * MARGIN + contentW);
    L.windowH = (int)(2 * MARGIN + headerH + GAP + row2H + GAP + bottomRowH + GAP + footerH);

    return L;
}

void ui::drawText(const Context &ctx, float x, float y, const std::string &text, Color color)
{
    if (text.empty()) {
        return;
    }
    SDL_SetRenderDrawColor(ctx.renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDebugText(ctx.renderer, x, y, text.c_str());
}

void ui::drawBox(SDL_Renderer *renderer, const SDL_FRect &rect, Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderRect(renderer, &rect);
}

void ui::drawDisplay(const Context &ctx, chip8 &c, const SDL_FRect &box)
{
    SDL_Renderer *renderer = ctx.renderer;
    drawBox(renderer, box, BRIGHT);
    const float offsetX = box.x + DISPLAY_PAD + BORDER;
    const float offsetY = box.y + DISPLAY_PAD + BORDER;

    SDL_FRect square = {.x = 0, .y = 0, .w = 10, .h = 10};
    for (uint8_t i = 0; i < 32; ++i) {
        for (uint8_t j = 0; j < 64; ++j) {
            square.x = offsetX + j * 10;
            square.y = offsetY + i * 10;
            if (c.getPixel(i, j)) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            }
            SDL_RenderFillRect(renderer, &square);

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderRect(renderer, &square);
        }
    }
}

void ui::drawHeader(const Context &ctx, const SDL_FRect &rect, const std::string &romName, int hz, bool paused)
{
    const std::string left = "CHIP-8   " + romName;
    drawText(ctx, rect.x, rect.y, left, BRIGHT);

    const std::string right = std::to_string(hz) + "Hz   " + (paused ? "PAUSE" : "RUN");
    drawText(ctx, rect.x + rect.w - textW(ctx, right), rect.y, right, BRIGHT);
}

void ui::drawCpuPanel(const Context &ctx, chip8 &c, const SDL_FRect &rect)
{
    const float x = rect.x + PANEL_PAD_X;
    float y = drawPanel(ctx, rect, "CPU");

    char buf[64];
    snprintf(buf, sizeof(buf), "PC %04X   I %04X", c.getPC(), c.getI());
    drawText(ctx, x, y, buf, BRIGHT);
    y += ctx.lineH;

    snprintf(buf, sizeof(buf), "SP %02X     OP %04X", c.getSP(), c.getOpcode());
    drawText(ctx, x, y, buf, BRIGHT);
    y += ctx.lineH;

    snprintf(buf, sizeof(buf), "DT %02X     ST %02X", c.getDelayTimer(), c.getSoundTimer());
    drawText(ctx, x, y, buf, BRIGHT);
}

void ui::drawStackPanel(const Context &ctx, chip8 &c, const SDL_FRect &rect)
{
    char label[32];
    snprintf(label, sizeof(label), "STACK   SP %02X/16", c.getSP());
    const float bodyY = drawPanel(ctx, rect, label);

    // 8 rows x 2 columns (depth = col*8 + row), matching the design doc's stackCols
    // generator: row r shows depths [r, r+8] side by side.
    const float colW = 9.0f * ctx.charW;
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 2; ++col) {
            const int depth = col * 8 + row;
            // SP alone can't distinguish an empty stack from one call deep (both read
            // SP==0), so treat depth==SP as used only when that slot holds a real
            // return address.
            const bool used = depth < c.getSP() || (depth == c.getSP() && c.getStack(depth) != 0);

            char text[12];
            if (used) {
                snprintf(text, sizeof(text), "%02d %04X", depth, c.getStack(depth));
            } else {
                snprintf(text, sizeof(text), "%02d ----", depth);
            }

            const float x = rect.x + PANEL_PAD_X + col * colW;
            const float y = bodyY + row * ctx.lineH;

            if (used && depth == c.getSP()) {
                const SDL_FRect hi{x - 2, y, 7 * ctx.charW + 4, ctx.charW};
                drawHighlightedText(ctx, hi, x, y, text);
            } else {
                drawText(ctx, x, y, text, used ? BRIGHT : DIM);
            }
        }
    }
}

void ui::drawKeypadPanel(const Context &ctx, chip8 &c, const SDL_FRect &rect)
{
    const float bodyY = drawPanel(ctx, rect, "KEYPAD");

    for (int i = 0; i < 16; ++i) {
        const int row = i / 4;
        const int col = i % 4;
        const SDL_FRect cell{
            rect.x + PANEL_PAD_X + col * (KEY_CELL + KEY_GAP),
            bodyY + row * (KEY_CELL + KEY_GAP),
            KEY_CELL, KEY_CELL
        };

        const bool pressed = c.getKey(KEY_LAYOUT[i].hex);
        if (pressed) {
            fillRect(ctx.renderer, cell, BRIGHT);
        }
        drawBox(ctx.renderer, cell, DIM);

        const Color hexColor = pressed ? BG : BRIGHT;
        const Color subColor = pressed ? BG : DIM;

        char hexCh[4];
        snprintf(hexCh, sizeof(hexCh), "%X", KEY_LAYOUT[i].hex);
        drawText(ctx, cell.x + (cell.w - ctx.charW) / 2, cell.y + 4, hexCh, hexColor);

        const std::string subLabel = std::string("(") + KEY_LAYOUT[i].label + ")";
        const float subW = textW(ctx, subLabel);
        drawText(ctx, cell.x + (cell.w - subW) / 2, cell.y + cell.h - ctx.charW - 4, subLabel, subColor);
    }
}

void ui::drawRegistersPanel(const Context &ctx, chip8 &c, const SDL_FRect &rect)
{
    const float bodyY = drawPanel(ctx, rect, "REGISTERS");
    const float colW = 6.5f * ctx.charW;

    for (int i = 0; i < 16; ++i) {
        const int col = i / 8;
        const int row = i % 8;
        char buf[16];
        snprintf(buf, sizeof(buf), "V%X %02X", i, c.getV(i));
        drawText(ctx, rect.x + PANEL_PAD_X + col * colW, bodyY + row * ctx.lineH, buf, BRIGHT);
    }
}

void ui::drawDisasmPanel(const Context &ctx, chip8 &c, const SDL_FRect &rect)
{
    const float bodyY = drawPanel(ctx, rect, "DISASM   > pc");
    const float x = rect.x + PANEL_PAD_X;

    // Window of instructions around PC, kept inside memory at either end.
    int start = (int)c.getPC() - 2 * DISASM_ROWS_BEFORE_PC;
    start = std::clamp(start, 0, 4096 - 2 * DISASM_ROWS);

    for (int i = 0; i < DISASM_ROWS; ++i) {
        const int addr = start + 2 * i;
        const uint16_t op = c.getMem(addr) << 8 | c.getMem(addr + 1);
        const float y = bodyY + i * ctx.lineH;
        const bool current = addr == (int)c.getPC();

        char text[64];
        snprintf(text, sizeof(text), "%s%04X  %04X  %s",
                 current ? "> " : "  ", addr, op, disassemble(op).c_str());

        if (current) {
            const SDL_FRect hi{x - 2, y, rect.w - 2 * PANEL_PAD_X + 4, ctx.charW};
            drawHighlightedText(ctx, hi, x, y, text);
        } else {
            drawText(ctx, x, y, text, BRIGHT);
        }
    }
}

void ui::drawMemoryPanel(const Context &ctx, chip8 &c, const SDL_FRect &rect)
{
    char label[32];
    snprintf(label, sizeof(label), "MEMORY   I %04X", c.getI());
    const float bodyY = drawPanel(ctx, rect, label);
    const float x = rect.x + PANEL_PAD_X;

    // Window of bytes around I, aligned to a row boundary and kept inside memory.
    const int aligned = (int)c.getI() & ~(MEM_BYTES - 1);
    const int base = std::clamp(aligned - 2 * MEM_BYTES, 0, 4096 - MEM_WINDOW);

    for (int row = 0; row < MEM_ROWS; ++row) {
        const int addr = base + row * MEM_BYTES;
        const float y = bodyY + row * ctx.lineH;

        char addrText[8];
        snprintf(addrText, sizeof(addrText), "%04X", addr);
        drawText(ctx, x, y, addrText, BRIGHT);

        for (int b = 0; b < MEM_BYTES; ++b) {
            const float bx = x + (6 + 3 * b) * ctx.charW;
            char byteText[4];
            snprintf(byteText, sizeof(byteText), "%02X", c.getMem(addr + b));

            if (addr + b == (int)c.getI()) {
                const SDL_FRect hi{bx - 2, y, 2 * ctx.charW + 4, ctx.charW};
                drawHighlightedText(ctx, hi, bx, y, byteText);
            } else {
                drawText(ctx, bx, y, byteText, BRIGHT);
            }
        }
    }
}

void ui::drawFooter(const Context &ctx, const SDL_FRect &rect)
{
    drawText(ctx, rect.x, rect.y, "F5 RESET   F7 STEP   SPACE PAUSE", DIM);
}
