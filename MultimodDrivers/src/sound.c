// sound.c
// Speaker + microphone driver for the Multimod AUDIO module. See sound.h.

#include "../sound.h"

#include <stdint.h>
#include <stdbool.h>

#include <inc/hw_memmap.h>
#include <inc/hw_ints.h>
#include <inc/hw_types.h>
#include <driverlib/sysctl.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/ssi.h>
#include <driverlib/timer.h>
#include <driverlib/adc.h>

#include "MultimodDrivers/multimod_i2c.h"
#include "G8RTOS/G8RTOS_CriticalSection.h"

/*************************************Defines***************************************/

#define DAC_SSI_BASE        SSI2_BASE
#define DAC_CS_PORT         GPIO_PORTF_BASE
#define DAC_CS_PIN          GPIO_PIN_1

#define AUDIO_EXP_ADDR      0x22        // PCA9555 on the audio module (I2C bus B)
#define EXP_OUTPUT0         0x02
#define EXP_CONFIG0         0x06
#define EXP_MIC_SHDN        (1 << 0)
#define EXP_DAC_LAT         (1 << 1)
#define EXP_SPK_SDZ         (1 << 4)

// MCP48FVB22 command byte: [AD4..AD0][C1 C0][x]; write = C1C0 = 00
#define DAC_REG_DAC0        0x00
#define DAC_REG_DAC1        0x01
#define DAC_CMD_WRITE(reg)  ((uint8_t)((reg) << 3))

#define DAC_MID             2048        // silence level (speaker is AC coupled)
#define NOISE_RATE_HZ       8000
#define REST_RATE_HZ        1000

/*********************************Private State*************************************/

static const SoundNote *noteList = 0;   // current list (0 = idle)
static volatile uint32_t ticksLeft = 0; // timer interrupts left in this note
static volatile uint8_t  noteKind = 0;  // 0 = rest, 1 = square, 2 = noise
static volatile uint8_t  level = 0;
static volatile uint16_t amplitude = 800;   // DAC counts above/below mid
static volatile bool     muted = false;
static uint32_t lfsr = 0xACE1u;

static SoundNote singleTone[2];

/*******************************Effect Definitions**********************************/

static const SoundNote sfxBeep[]     = { {NOTE_A5, 30}, {0, 0} };
static const SoundNote sfxSelect[]   = { {NOTE_C5, 60}, {NOTE_G5, 90}, {0, 0} };
static const SoundNote sfxHit[]      = { {NOTE_E5, 25}, {0, 0} };
static const SoundNote sfxBreak[]    = { {NOTE_A5, 30}, {NOTE_C6, 40}, {0, 0} };
static const SoundNote sfxEat[]      = { {NOTE_C5, 30}, {NOTE_E5, 30}, {0, 0} };
static const SoundNote sfxPowerup[]  = { {NOTE_C5, 50}, {NOTE_E5, 50}, {NOTE_G5, 50}, {NOTE_C6, 90}, {0, 0} };
static const SoundNote sfxShoot[]    = { {1400, 20}, {1100, 20}, {800, 20}, {0, 0} };
static const SoundNote sfxExplode[]  = { {SOUND_NOISE, 180}, {0, 0} };
static const SoundNote sfxFlap[]     = { {NOTE_G4, 25}, {NOTE_C5, 25}, {0, 0} };
static const SoundNote sfxPoint[]    = { {NOTE_E5, 50}, {NOTE_A5, 80}, {0, 0} };
static const SoundNote sfxLoseLife[] = { {NOTE_G4, 120}, {NOTE_E4, 120}, {NOTE_C4, 200}, {0, 0} };
static const SoundNote sfxGameOver[] = { {NOTE_C5, 150}, {NOTE_G4, 150}, {NOTE_E4, 150},
                                         {NOTE_A4, 120}, {NOTE_B4, 120}, {NOTE_A4, 120},
                                         {NOTE_G4, 300}, {0, 0} };
static const SoundNote sfxWin[]      = { {NOTE_C5, 100}, {NOTE_E5, 100}, {NOTE_G5, 100},
                                         {0, 50}, {NOTE_E5, 100}, {NOTE_G5, 100}, {NOTE_C6, 250}, {0, 0} };

static const SoundNote *const sfxTable[SFX_COUNT] = {
    sfxBeep, sfxSelect, sfxHit, sfxBreak, sfxEat, sfxPowerup, sfxShoot,
    sfxExplode, sfxFlap, sfxPoint, sfxLoseLife, sfxGameOver, sfxWin
};

/********************************Private Functions**********************************/

static void Expander_Write(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = { reg, value };
    I2C_WriteMultiple(I2C_B_BASE, AUDIO_EXP_ADDR, buf, 2);
}

static inline void SSI_Put(uint8_t b) {
    SSIDataPut(DAC_SSI_BASE, b);
}

// Writes the same 12-bit value to both DAC channels (left and right input of the amp)
static void DAC_Write(uint16_t value) {
    value &= 0x0FFF;
    GPIOPinWrite(DAC_CS_PORT, DAC_CS_PIN, 0);
    SSI_Put(DAC_CMD_WRITE(DAC_REG_DAC0));
    SSI_Put(value >> 8);
    SSI_Put(value & 0xFF);
    while (SSIBusy(DAC_SSI_BASE));
    GPIOPinWrite(DAC_CS_PORT, DAC_CS_PIN, DAC_CS_PIN);    // CS high -> output updates

    GPIOPinWrite(DAC_CS_PORT, DAC_CS_PIN, 0);
    SSI_Put(DAC_CMD_WRITE(DAC_REG_DAC1));
    SSI_Put(value >> 8);
    SSI_Put(value & 0xFF);
    while (SSIBusy(DAC_SSI_BASE));
    GPIOPinWrite(DAC_CS_PORT, DAC_CS_PIN, DAC_CS_PIN);

    // empty the RX FIFO so it never fills up
    uint32_t dummy;
    while (SSIDataGetNonBlocking(DAC_SSI_BASE, &dummy));
}

// Sets up the timer for the note at noteList[0]; returns false at the end of the list.
// Called with interrupts off (from the ISR or inside a critical section).
static bool StartNote(void) {
    if (noteList == 0 || noteList->ms == 0) {
        noteList = 0;
        return false;
    }

    uint32_t clk = SysCtlClockGet();
    uint32_t rate;

    if (noteList->freq == 0) {                    // rest
        noteKind = 0;
        rate = REST_RATE_HZ;
    } else if (noteList->freq == SOUND_NOISE) {   // noise
        noteKind = 2;
        rate = NOISE_RATE_HZ;
    } else {                                      // square wave: 2 interrupts per cycle
        noteKind = 1;
        rate = 2u * noteList->freq;
    }

    ticksLeft = (rate * noteList->ms) / 1000u;
    if (ticksLeft == 0) ticksLeft = 1;
    TimerLoadSet(TIMER1_BASE, TIMER_A, clk / rate - 1);
    return true;
}

/********************************Public Functions***********************************/

void Sound_Init(void) {
    // ---- audio module control pins via PCA9555 @0x22 (I2C bus B) ----
    I2C_Init(I2C_B_BASE);
    Expander_Write(EXP_OUTPUT0, EXP_MIC_SHDN | EXP_SPK_SDZ);  // mic on, amp on, DAC_LAT low
    Expander_Write(EXP_CONFIG0, (uint8_t)~(EXP_MIC_SHDN | EXP_DAC_LAT | EXP_SPK_SDZ)); // those 3 = outputs
    // The TPA2016D2 (0x58) powers up with both speaker channels enabled, so
    // it needs no I2C setup once SDZ is high.

    // ---- DAC chip select (PF1) ----
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));
    GPIOPinTypeGPIOOutput(DAC_CS_PORT, DAC_CS_PIN);
    GPIOPinWrite(DAC_CS_PORT, DAC_CS_PIN, DAC_CS_PIN);        // deselected

    // ---- SSI2 on PB4 (CLK), PB6 (RX), PB7 (TX) ----
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI2));
    GPIOPinConfigure(GPIO_PB4_SSI2CLK);
    GPIOPinConfigure(GPIO_PB6_SSI2RX);
    GPIOPinConfigure(GPIO_PB7_SSI2TX);
    GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_7);

    SSIDisable(DAC_SSI_BASE);
    SSIClockSourceSet(DAC_SSI_BASE, SSI_CLOCK_SYSTEM);
    // MCP48FVBxx: SPI mode 0, up to 20 MHz. 10 MHz needs SysClk >= 20 MHz (use 80 MHz).
    SSIConfigSetExpClk(DAC_SSI_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0,
                       SSI_MODE_MASTER, 10000000, 8);
    SSIEnable(DAC_SSI_BASE);

    DAC_Write(DAC_MID);                                       // silence

    // ---- Timer1A: periodic, interrupt rate set per note ----
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER1));
    TimerDisable(TIMER1_BASE, TIMER_A);
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER1_BASE, TIMER_A, SysCtlClockGet() / REST_RATE_HZ - 1);
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    // NVIC side (vector, priority, enable) is done by G8RTOS_Add_APeriodicEvent
}

// Timer1A interrupt: one step of the current note.
void Sound_TimerISR(void) {
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    if (ticksLeft == 0) {
        if (noteList) noteList++;
        if (!StartNote()) {                       // end of list -> silence and stop
            DAC_Write(DAC_MID);
            TimerDisable(TIMER1_BASE, TIMER_A);
            return;
        }
    }
    ticksLeft--;

    uint16_t amp = muted ? 0 : amplitude;
    uint16_t out = DAC_MID;
    if (noteKind == 1) {                          // square wave
        level ^= 1;
        out = level ? DAC_MID + amp : DAC_MID - amp;
    } else if (noteKind == 2) {                   // white noise (16-bit LFSR)
        lfsr = (lfsr >> 1) ^ (-(int32_t)(lfsr & 1u) & 0xB400u);
        out = (lfsr & 1) ? DAC_MID + amp : DAC_MID - amp;
    }
    DAC_Write(out);
}

void Sound_Play(const SoundNote *notes) {
    int32_t ibit = StartCriticalSection();
    TimerDisable(TIMER1_BASE, TIMER_A);
    noteList = notes;
    if (StartNote()) {
        TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
        TimerEnable(TIMER1_BASE, TIMER_A);
    }
    EndCriticalSection(ibit);
}

void Sound_Tone(uint16_t freq, uint16_t ms) {
    int32_t ibit = StartCriticalSection();
    singleTone[0].freq = freq;
    singleTone[0].ms = ms;
    singleTone[1].freq = 0;
    singleTone[1].ms = 0;
    EndCriticalSection(ibit);
    Sound_Play(singleTone);
}

void Sound_Effect(SoundEffect sfx) {
    if (sfx < SFX_COUNT) Sound_Play(sfxTable[sfx]);
}

void Sound_Stop(void) {
    int32_t ibit = StartCriticalSection();
    TimerDisable(TIMER1_BASE, TIMER_A);
    noteList = 0;
    ticksLeft = 0;
    DAC_Write(DAC_MID);
    EndCriticalSection(ibit);
}

bool Sound_IsPlaying(void) {
    return noteList != 0;
}

void Sound_SetVolume(uint8_t volume) {
    if (volume > 100) volume = 100;
    amplitude = (uint16_t)((2000u * volume) / 100u);      // max swing +/-2000 counts
}

void Sound_SetMute(bool mute) {
    muted = mute;
}

/*********************************Microphone****************************************/

void Mic_Init(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0));
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE));
    GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_1);              // AIN2

    // Sequencer 3 (1 step); the joystick uses sequencer 1, so they don't clash
    ADCSequenceDisable(ADC0_BASE, 3);
    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 1);
    ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH2 | ADC_CTL_IE | ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE, 3);
    ADCIntClear(ADC0_BASE, 3);
}

uint16_t Mic_Read(void) {
    uint32_t v;
    ADCIntClear(ADC0_BASE, 3);
    ADCProcessorTrigger(ADC0_BASE, 3);
    while (!ADCIntStatus(ADC0_BASE, 3, false));
    ADCIntClear(ADC0_BASE, 3);
    ADCSequenceDataGet(ADC0_BASE, 3, &v);
    return (uint16_t)(v & 0xFFF);
}

uint16_t Mic_GetLevel(void) {
    uint16_t lo = 4095, hi = 0;
    for (uint8_t i = 0; i < 32; i++) {
        uint16_t s = Mic_Read();
        if (s < lo) lo = s;
        if (s > hi) hi = s;
        SysCtlDelay(50);                                      // ~2 us apart at 80 MHz
    }
    return hi - lo;
}
