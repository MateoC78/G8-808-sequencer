// audio.h

#ifndef AUDIO_H_
#define AUDIO_H_

#include <stdint.h>

#define SAMPLE_RATE_HZ      16000       // DAC samples per second
#define DAC_MID             2048        // 12-bit mid-scale = silence

void Audio_Init(void);                  // call before G8RTOS_Launch (uses I2C + SPI directly)
void Audio_Start(void);                 // start the 16 kHz Timer1A interrupt
void Audio_Stop(void);
void Audio_DACWrite(uint16_t value);    // 0..4095 to both DAC channels (safe inside the ISR)

#endif