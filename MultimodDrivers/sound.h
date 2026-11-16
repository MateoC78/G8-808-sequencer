// sound.h
// Speaker driver for the Multimod AUDIO module + simple sound effects.
//
// Hardware (Multimod schematic pages 10 and 13):
//   DAC   : MCP48FVB22 (dual 12-bit, SPI) on SPI bus B = SSI2
//           PB4 = SSI2CLK, PB7 = SSI2TX (MOSI), PB6 = SSI2RX (MISO)
//           CS  = DAC_SPICS = PF1 (GPIO)        <- also the Launchpad red LED:
//                                                  don't call LaunchpadLED_Init()
//   Amp   : TPA2016D2 class-D amplifier, I2C bus B (I2C0, PB2/PB3), addr 0x58
//   GPIO  : PCA9555 expander at 0x22 on I2C bus B
//           IO0_0 = AMPMIC_SHDN (mic amp on = 1)
//           IO0_1 = DAC_LAT     (hold 0 -> DAC output updates when CS goes high)
//           IO0_4 = AMPSPK_SDZ  (speaker amp shutdown, active low -> drive 1)
//
// How it plays: a hardware timer (Timer1A) interrupts at 2x the note frequency
// and writes a high/low level to the DAC -> square wave. Notes are played from
// a list, so a thread just calls Sound_Effect(SFX_xxx) and keeps going.
//
// Setup (in main):
//   Sound_Init();                                            // after G8RTOS_Init()
//   G8RTOS_Add_APeriodicEvent(Sound_TimerISR, 3, INT_TIMER1A);

#ifndef SOUND_H_
#define SOUND_H_

#include <stdint.h>
#include <stdbool.h>

// One note: frequency in Hz (0 = rest, SOUND_NOISE = noise) and length in ms.
// A list of notes ends with {0, 0}.
typedef struct {
    uint16_t freq;
    uint16_t ms;
} SoundNote;

#define SOUND_NOISE     0xFFFF

// Musical notes (Hz)
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_C6  1047

// Ready-made effects
typedef enum {
    SFX_BEEP = 0,       // menu move
    SFX_SELECT,         // menu select / start
    SFX_HIT,            // ball hits paddle / wall
    SFX_BREAK,          // brick broken
    SFX_EAT,            // snake food / pellet
    SFX_POWERUP,        // power pellet / bonus
    SFX_SHOOT,          // laser
    SFX_EXPLODE,        // enemy destroyed / crash
    SFX_FLAP,           // flappy bird flap
    SFX_POINT,          // scored a point
    SFX_LOSE_LIFE,
    SFX_GAMEOVER,
    SFX_WIN,
    SFX_COUNT
} SoundEffect;

void Sound_Init(void);
void Sound_TimerISR(void);                       // aperiodic event on INT_TIMER1A
void Sound_Tone(uint16_t freq, uint16_t ms);     // single tone, non-blocking
void Sound_Play(const SoundNote *notes);         // note list ending in {0,0}, non-blocking
void Sound_Effect(SoundEffect sfx);
void Sound_Stop(void);
bool Sound_IsPlaying(void);
void Sound_SetVolume(uint8_t volume);            // 0..100 (default 40)
void Sound_SetMute(bool mute);

// Optional: microphone (MAX9814 on PE1 / AIN2), e.g. "blow to flap"
void Mic_Init(void);                             // uses ADC0 sequencer 3
uint16_t Mic_Read(void);                         // one raw sample 0..4095
uint16_t Mic_GetLevel(void);                     // loudness: peak-to-peak of 32 samples

#endif /* SOUND_H_ */
