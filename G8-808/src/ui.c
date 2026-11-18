

#include "ui.h"
#include "synth.h"
#include "./MultimodDrivers/multimod.h"



void UI_Rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (x < 0) { w += x; x = 0; }                   // clip to the screen
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (w <= 0 || h <= 0) return;
    ST7789_DrawRectangle(x, y, w, h, color);
}