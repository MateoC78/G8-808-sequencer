// ui.h
// G8-808 screen: step grid, playhead and status line on the ST7789 LCD.
// Call these while holding sem_LCD.

#ifndef UI_H_
#define UI_H_

#include <stdint.h>
#include <stdbool.h>

// colors (the ST7789 here is BGR565)
#define UI_BLACK        0x0000
#define UI_WHITE        0xFFFF
#define UI_GRAY         0x8410
#define UI_DARKGRAY     0x4208
#define UI_DIM          0x2104
#define UI_RED          0x001F
#define UI_GREEN        0x07E0
#define UI_YELLOW       0x07FF
#define UI_ORANGE       0x053F

#define SCREEN_W        240
#define SCREEN_H        320

#define GRID_X          44          // left edge of step 0
#define GRID_Y          80
#define STEP_W          12
#define ROW_H           18
#define CELL_W          10
#define CELL_H          14
#define HEAD_Y          70
#define HEAD_H          5

void UI_Rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void UI_Text(int16_t x, int16_t y, const char *s, uint8_t scale, uint16_t color, uint16_t bg);
void UI_Number(int16_t x, int16_t y, uint32_t n, uint8_t digits, uint8_t scale, uint16_t color, uint16_t bg);

void UI_DrawScreen(void);                                  
void UI_DrawCell(uint8_t track, uint8_t step, bool on, bool accent);
void UI_DrawTrack(uint8_t track, uint16_t steps, uint16_t accent);
void UI_DrawPlayhead(int8_t step);                  
void UI_DrawStatus(bool playing, uint16_t bpm, uint32_t underruns);

#endif
