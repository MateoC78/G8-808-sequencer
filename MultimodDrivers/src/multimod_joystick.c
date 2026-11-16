#include "../multimod_joystick.h"
 
#include <stdint.h>
#include <stdbool.h>
 
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
 
#include <driverlib/sysctl.h>
#include <driverlib/gpio.h>
#include <driverlib/adc.h>

void JOYSTICK_Init(){
    // Initialization code for the joystick
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); // Enable ADC0 peripheral
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0));

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE));

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD));
    
    GPIOPinTypeADC(JOYSTICK_XY_GPIO_BASE, JOYSTICK_X_PIN | JOYSTICK_Y_PIN);

    ADCSequenceDisable(JOYSTICK_ADC_BASE, JOYSTICK_ADC_SEQ);
    ADCSequenceConfigure(JOYSTICK_ADC_BASE, JOYSTICK_ADC_SEQ, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(JOYSTICK_ADC_BASE, JOYSTICK_ADC_SEQ, 0, ADC_CTL_CH0);                         // X
    ADCSequenceStepConfigure(JOYSTICK_ADC_BASE, JOYSTICK_ADC_SEQ, 1, ADC_CTL_CH1 | ADC_CTL_IE | ADC_CTL_END); // Y, last step
    ADCHardwareOversampleConfigure(JOYSTICK_ADC_BASE, 4);   // average 4 samples in HW (less jitter)
    ADCSequenceEnable(JOYSTICK_ADC_BASE, JOYSTICK_ADC_SEQ);
    ADCIntClear(JOYSTICK_ADC_BASE, JOYSTICK_ADC_SEQ);
 
    GPIOPinTypeGPIOInput(JOYSTICK_SW_GPIO_BASE, JOYSTICK_SW_PIN);
    GPIOPadConfigSet(JOYSTICK_SW_GPIO_BASE, JOYSTICK_SW_PIN,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}

uint8_t JOYSTICK_IntEnable(){
    GPIOIntDisable(JOYSTICK_SW_GPIO_BASE, JOYSTICK_SW_INT_PIN);
    GPIOIntTypeSet(JOYSTICK_SW_GPIO_BASE, JOYSTICK_SW_PIN, GPIO_FALLING_EDGE);
    GPIOIntClear(JOYSTICK_SW_GPIO_BASE, JOYSTICK_SW_INT_PIN); 
    GPIOIntEnable(JOYSTICK_SW_GPIO_BASE, JOYSTICK_SW_INT_PIN);
    return 0; 
}

int32_t JOYSTICK_GETXY(){
    int32_t data[4];
 
    ADCIntClear(JOYSTICK_ADC_BASE, JOYSTICK_ADC_SEQ);
    ADCProcessorTrigger(JOYSTICK_ADC_BASE, JOYSTICK_ADC_SEQ);
    while (!ADCIntStatus(JOYSTICK_ADC_BASE, JOYSTICK_ADC_SEQ, false));
    ADCIntClear(JOYSTICK_ADC_BASE, JOYSTICK_ADC_SEQ);
    ADCSequenceDataGet(JOYSTICK_ADC_BASE, JOYSTICK_ADC_SEQ, data);
 
    int32_t x = data[0] & 0xFFF;
    int32_t y = data[1] & 0xFFF;
    return (x << 16) | y;
}
