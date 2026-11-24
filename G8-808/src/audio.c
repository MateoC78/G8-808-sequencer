// audio.c - G8-808 audio output using the provided Multimod DAC driver
#include "../audio.h"

#include <stdint.h>
#include <stdbool.h>
#include <inc/hw_memmap.h>
#include <driverlib/sysctl.h>
#include <driverlib/timer.h>

#include "../../MultimodDrivers/multimod_dac.h"

void Audio_Init(void) {
    MultimodDAC_Init();                         // I2C0, expander, SSI2, PF1, Timer1 (starts it at 200 Hz)

    TimerDisable(TIMER1_BASE, TIMER_A);         // stop it: Mix_Thread starts it when the RTOS is running
    TimerLoadSet(TIMER1_BASE, TIMER_A, SysCtlClockGet() / SAMPLE_RATE_HZ - 1);   // 16 kHz
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    MultimodDAC_Write(DAC_OUT_REG, DAC_MID);    // silence
}

void Audio_Start(void) { TimerEnable(TIMER1_BASE, TIMER_A); }

void Audio_Stop(void) {
    TimerDisable(TIMER1_BASE, TIMER_A);
    MultimodDAC_Write(DAC_OUT_REG, DAC_MID);
}

void Audio_DACWrite(uint16_t value) {
    if (value > 4095) value = 4095;
    MultimodDAC_Write(DAC_OUT_REG, value);
}
