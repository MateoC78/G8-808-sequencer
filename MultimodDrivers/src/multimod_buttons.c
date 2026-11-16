#include "../multimod_buttons.h"
#include <stdint.h>
#include "../multimod_i2c.h"
#include <driverlib/sysctl.h>
#include <driverlib/gpio.h>


void MultimodButtons_Init(){
    // PCA9555 init for tactile switches
    I2C_Init(I2C_A_BASE);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE));
    GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_4);
    GPIOPadConfigSet(GPIO_PORTE_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    GPIOIntTypeSet(GPIO_PORTE_BASE, GPIO_PIN_4, GPIO_LOW_LEVEL);

    MultimodButtons_Get();
    GPIOIntClear(GPIO_PORTE_BASE, GPIO_INT_PIN_4);
    GPIOIntEnable(GPIO_PORTE_BASE, GPIO_INT_PIN_4);
}

uint8_t MultimodButtons_Get() {
    uint8_t reg = 0x00; 
    uint8_t data;
    // Read the input port register (0x00) from PCA9555
    I2C_WriteMultiple(I2C_A_BASE, PCA9555_ADDR, &reg, 1);
    I2C_ReadMultiple(I2C_A_BASE, PCA9555_ADDR, &data, 1);

    return ((uint8_t)(~data) & 0x1E);  // Mask to get only the relevant bits for switches A, B, C, D
}
