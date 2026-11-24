

#include "../ui.h"
#include "../synth.h"
#include "./MultimodDrivers/multimod.h"


static const char *const trackNames[NUM_TRACKS] = { "KICK", "SNAR", "CHAT", "OHAT", "CLAP", "TOM" };

// 3x5 font: 5 rows per character, bit 2 = left column
static const uint8_t font3x5[][5] = {
    {0,0,0,0,0},            // ' '
    {7,5,5,5,7}, {2,6,2,2,7}, {7,1,7,4,7}, {7,1,7,1,7}, {5,5,7,1,1},   // 0-4
    {7,4,7,1,7}, {7,4,7,5,7}, {7,1,1,1,1}, {7,5,7,5,7}, {7,5,7,1,7},   // 5-9
    {2,5,7,5,5}, {6,5,6,5,6}, {3,4,4,4,3}, {6,5,5,5,6}, {7,4,6,4,7},   // A-E
    {7,4,6,4,4}, {3,4,5,5,3}, {5,5,7,5,5}, {7,2,2,2,7}, {1,1,1,5,2},   // F-J
    {5,5,6,5,5}, {4,4,4,4,7}, {5,7,5,5,5}, {6,5,5,5,5}, {2,5,5,5,2},   // K-O
    {6,5,6,4,4}, {2,5,5,6,3}, {6,5,6,5,5}, {3,4,2,1,6}, {7,2,2,2,2},   // P-T
    {5,5,5,5,7}, {5,5,5,5,2}, {5,5,5,7,5}, {5,5,2,5,5}, {5,5,2,2,2},   // U-Y
    {7,1,2,4,7},                                                        // Z
    {0,0,7,0,0},            // '-'
    {0,2,0,2,0},            // ':'
};

static uint8_t FontIndex(char c) {
    if (c >= 'a' && c <= 'z') c -= 32;
    if (c >= '0' && c <= '9') return 1 + (c - '0');
    if (c >= 'A' && c <= 'Z') return 11 + (c - 'A');
    if (c == '-') return 37;
    if (c == ':') return 38;
    return 0;
}

void UI_Rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (x < 0) { w += x; x = 0; }                   // clip to the screen
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (w <= 0 || h <= 0) return;
    y = SCREEN_H - y - h;
    ST7789_DrawRectangle(x, y, w, h, color);
}

void UI_Text(int16_t x, int16_t y, const char *s, uint8_t scale, uint16_t color, uint16_t bg) {
    uint8_t row, col;
    while (*s) {
        const uint8_t *g = font3x5[FontIndex(*s)];
        UI_Rect(x, y, 4 * scale, 5 * scale, bg);
        for (row = 0; row < 5; row++)
            for (col = 0; col < 3; col++)
                if ((g[row] >> (2 - col)) & 1)
                    UI_Rect(x + col * scale, y + row * scale, scale, scale, color);
        x += 4 * scale;
        s++;
    }
}

void UI_Number(int16_t x, int16_t y, uint32_t n, uint8_t digits, uint8_t scale, uint16_t color, uint16_t bg) {
    char tmp[10], buf[11];
    uint8_t len = 0, i;
    do { tmp[len++] = '0' + (n % 10); n /= 10; } while (n && len < 10);
    while (len < digits && len < 10) tmp[len++] = ' ';       // pad on the left with spaces
    for (i = 0; i < len; i++) buf[i] = tmp[len - 1 - i];
    buf[len] = 0;
    UI_Text(x, y, buf, scale, color, bg);
}

void UI_DrawCell(uint8_t track, uint8_t step, bool on, bool accent) {
    uint16_t color;
    if (on)                   color = accent ? UI_ORANGE : UI_WHITE;
    else if ((step & 3) == 0) color = UI_DARKGRAY;           // first step of each beat is a bit brighter
    else                      color = UI_DIM;
    UI_Rect(GRID_X + step * STEP_W, GRID_Y + track * ROW_H, CELL_W, CELL_H, color);
}

void UI_DrawTrack(uint8_t track, uint16_t steps, uint16_t accent) {
    uint8_t s;
    for (s = 0; s < 16; s++)
        UI_DrawCell(track, s, (steps >> s) & 1, (accent >> s) & 1);
}

void UI_DrawPlayhead(int8_t step) {
    static int8_t shown = -1;
    if (step == shown) return;
    if (shown >= 0) UI_Rect(GRID_X + shown * STEP_W, HEAD_Y, CELL_W, HEAD_H, UI_BLACK);
    if (step >= 0)  UI_Rect(GRID_X + step * STEP_W, HEAD_Y, CELL_W, HEAD_H, UI_GREEN);
    shown = step;
}

void UI_DrawStatus(bool playing, uint16_t bpm, uint32_t underruns) {
    UI_Text(184, 10, playing ? "PLAY" : "STOP", 3, playing ? UI_GREEN : UI_RED, UI_BLACK);
    UI_Number(40, 40, bpm, 3, 2, UI_WHITE, UI_BLACK);
    UI_Number(196, 40, underruns, 4, 2, underruns ? UI_RED : UI_WHITE, UI_BLACK);
}

void UI_DrawScreen(void) {
    uint8_t t, beat;
    ST7789_Fill(UI_BLACK);
    UI_Text(8, 6, "G8-808", 4, UI_ORANGE, UI_BLACK);
    UI_Text(8, 40, "BPM", 2, UI_GRAY, UI_BLACK);
    UI_Text(156, 40, "XRUN", 2, UI_GRAY, UI_BLACK);
    UI_Rect(0, 60, SCREEN_W, 2, UI_DARKGRAY);

    for (t = 0; t < NUM_TRACKS; t++) {
        UI_Text(4, GRID_Y + t * ROW_H + 2, trackNames[t], 2, UI_GRAY, UI_BLACK);
        UI_DrawTrack(t, 0, 0);
    }
    for (beat = 0; beat < 4; beat++)                         // beat numbers under the grid
        UI_Number(GRID_X + beat * 4 * STEP_W + 2, GRID_Y + NUM_TRACKS * ROW_H + 2, beat + 1, 1, 2, UI_GRAY, UI_BLACK);

    UI_Text(8, 232, "SW1 PLAY-STOP", 2, UI_GRAY, UI_BLACK);
    UI_Text(8, 248, "SW2 SLOWER  SW3 FASTER", 2, UI_GRAY, UI_BLACK);
    UI_Text(8, 264, "SW4 NEXT PATTERN", 2, UI_GRAY, UI_BLACK);
}
