#ifndef MULTIMOD_PCA9956B_H_
#define MULTIMOD_PCA9956B_H_

#include <stdint.h>

#define PCA9956B_ADDR           0x01
 
#define PCA9956B_MODE1          0x00
#define PCA9956B_MODE2          0x01
#define PCA9956B_LEDOUT0        0x02    // LEDOUT0..5 = 0x02..0x07
#define PCA9956B_PWM0           0x0A    // PWM0..23   = 0x0A..0x21
#define PCA9956B_IREF0          0x22    // IREF0..23  = 0x22..0x39
#define PCA9956B_PWMALL         0x3F
#define PCA9956B_IREFALL        0x40
#define PCA9956B_AIF            0x80    // Auto-Increment Flag in the control byte
 
#define PCA9956B_NUM_LEDS       20      // D00..D19 on the board
#define PCA9956B_FIRST_OUTPUT   2       // D00 is on output LED2
 
// Output current per LED: I = IREF/255 * ~20 mA (REXT = 2.87k on the board)
#define PCA9956B_DEFAULT_IREF   0x40

void PCA9956b_Init(void);
void PCA9956b_WriteRegister(uint8_t reg, uint8_t value);
void PCA9956b_SetLED(uint8_t led, uint8_t brightness);              // led = 0..19 (D00..D19)
void PCA9956b_Clear(void);
 
#endif