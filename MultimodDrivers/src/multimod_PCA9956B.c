#include "../multimod_PCA9956b.h"
#include "../multimod_i2c.h"
#include "../multimod_buttons.h"        // PCA9555_ADDR
 
#include <stdint.h>
#include <stdbool.h>
#include <inc/hw_memmap.h>
#include <driverlib/sysctl.h>
#include <driverlib/gpio.h>

#define PCA9956B_OUTPUT0         0x02
#define PCA9956B_CONFIG0         0x06

void PCA9956b_Init(void) {
    // ---- /OE on PB5: drive LOW to enable the LED outputs ----
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_5);
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5, 0);
 
    // ---- /RESET via PCA9555 IO0_0: make it an output and pulse it ----
    // IO0_1..IO0_4 (the tactile buttons) stay inputs.
    PCA9956b_WriteRegister(PCA9956B_OUTPUT0, 0xFF);     // IO0_0 high before enabling as output
    PCA9956b_WriteRegister(PCA9956B_CONFIG0, 0xFE);     // IO0_0 = output, rest = inputs
    PCA9956b_WriteRegister(PCA9956B_OUTPUT0, 0xFE);     // /RESET low  -> reset PCA9956B
    SysCtlDelay(SysCtlClockGet() / 3000);             // ~1 ms
    PCA9956b_WriteRegister(PCA9956B_OUTPUT0, 0xFF);     // /RESET high -> run
    SysCtlDelay(SysCtlClockGet() / 3000);             // ~1 ms
 
    // ---- PCA9956B setup ----
    PCA9956b_WriteRegister(PCA9956B_MODE1, 0x00);     // normal mode (SLEEP = 0), AI1:AI0 = 00
 
    // Every output in individual-PWM mode (LDRx = 10 -> 0xAA per register)
    for (uint8_t i = 0; i < 6; i++) {
        PCA9956b_WriteRegister(PCA9956B_LEDOUT0 + i, 0xAA);
    }
 
    // IREF defaults to 0 -> LEDs stay dark until a current is set
    PCA9956b_WriteRegister(PCA9956B_IREFALL, PCA9956B_DEFAULT_IREF);
    PCA9956b_WriteRegister(PCA9956B_PWMALL, 0x00);    // all off
}

void PCA9956b_WriteRegister(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = { reg, value };
    I2C_WriteMultiple(I2C_A_BASE, PCA9956B_ADDR, buf, 2);
}

void PCA9956b_SetLED(uint8_t led, uint8_t brightness) {
    if (led >= PCA9956B_NUM_LEDS) return;
    PCA9956b_WriteRegister(PCA9956B_PWM0 + PCA9956B_FIRST_OUTPUT + led, brightness);
}


void PCA9956b_Clear(void) {
    PCA9956b_WriteRegister(PCA9956B_PWMALL, 0x00);
}