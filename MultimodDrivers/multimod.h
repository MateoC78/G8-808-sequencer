#ifndef MULTIMOD_H_
#define MULTIMOD_H_

#include "multimod_uart.h"
#include "multimod_i2c.h"
#include "multimod_BMI160.h"
#include "multimod_OPT3001.h"
#include "multimod_LaunchpadButtons.h"
#include "multimod_LaunchpadLED.h"
#include "multimod_joystick.h"
#include "multimod_buttons.h"
#include "multimod_ST7789.h"
#include "multimod_spi.h"
#include "multimod_PCA9956B.h"
#include "multimod_dac.h"
#include "multimod_PCA9555.h"


static void Multimod_Init() {
    BMI160_Init();
    //OPT3001_Init();
    UART_Init();
    //LaunchpadButtons_Init();
    //LaunchpadLED_Init();
    JOYSTICK_Init();
    MultimodButtons_Init();
    PCA9956b_Init();    //multimod LED matrix
    MultimodDAC_Init();
    ST7789_Init();
}

#endif /* MULTIMOD_H_ */
