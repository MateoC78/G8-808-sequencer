// multimod_OPT3001.c
// Date Created: 2023-07-25
// Date Updated: 2023-07-27
// Defines for OPT3001 functions

/************************************Includes***************************************/

#include "../multimod_LaunchpadButtons.h"

#include <stdint.h>
#include <stdbool.h>

#include <inc/tm4c123gh6pm.h>
#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <inc/hw_i2c.h>
#include <inc/hw_gpio.h>

#include <driverlib/gpio.h>
#include <driverlib/sysctl.h>
#include <driverlib/uart.h>

/************************************Includes***************************************/

/********************************Public Functions***********************************/

// LaunchpadButtons_Init
// Initializes the GPIO port & pins necessary for the button switches on the
// launchpad.
// Return: void
void LaunchpadButtons_Init() {
    // Enable port F for switches
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);


    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= GPIO_PIN_0;

    // Use SW1 & SW2, configure as inputs.
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4 | GPIO_PIN_0);

    GPIOPadConfigSet(GPIO_PORTF_BASE,
                     GPIO_PIN_4 | GPIO_PIN_0,
                     GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD_WPU);

    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
}

// LaunchpadButtons_ReadSW1
// Checks to see if button 1 is pressed. If it is, return 1; else return 0.
// Return: uint8_t
uint8_t LaunchpadButtons_ReadSW1() {
    // GPIOPinRead() may be of use

    // your code here
    return (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0) ? 1 : 0;

}

// LaunchpadButtons_ReadSW2
// Checks to see if button 2 is pressed. If it is, return 1; else return 0.
// Return: uint8_t
uint8_t LaunchpadButtons_ReadSW2() {
    // your code here
    return (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0) ? 1 : 0;
}



/********************************Public Functions***********************************/
