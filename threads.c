// threads.c
// Date Created: 2026-11-16
// Defines for thread functions.

/************************************Includes***************************************/

#include "./threads.h"

#include "./MultimodDrivers/multimod.h"

#include "inc/hw_memmap.h"
#include "driverlib/timer.h"
#include "driverlib/adc.h"


/*********************************Global Variables**********************************/
Pattern pattern;
Sequencer seq;
volatile uint32_t underruns = 0;    // DAC ready to output block that isn't ready

// Ping Pong buffer
static uint16_t audioBuf[2][BLOCK_SIZE];
static volatile uint8_t playHalf = 0;
static volatile uint8_t playIndex = 0;
static volatile bool blockReady = true;



/*********************************Global Variables**********************************/

/********************************Public Functions***********************************/


/*************************************Threads***************************************/

void Idle_Thread(void) {
    while(1);
}

void Mix_Thread(void) {
    bool wasPlaying = false;
    uint8_t i, t;

    Synth_Init();
    TimerEnable(TIMER1_BASE, TIMER_A);      // start 16 kHz interrupt
    
    while (1) {
        G8RTOS_WaitSemaphore(&sem_mix);     // a half just finished playing

        uint16_t *out = audioBuf[playHalf ^ 1];     // half that is not playing
        uint32_t samplesPerStep = (SAMPLE_RATE_HZ * 15) / seq.bpm;
        bool playing = seq.playing;

        if (playing && !wasPlaying) {
            seq.step = 0;
            seq.sampleCount = 0;
        }
        wasPlaying = playing;

        for (i = 0; i < BLOCK_SIZE; i++) {
            if (playing) {
                if (seq.sampleCount == 0) {
                    uint16_t bit = (uint16_t)(1u << seq.step);
                    for (uint8_t j = 0; j < NUM_TRACKS; j++) {
                        if (pattern.steps[j] & bit) Synth_Trigger(j, (pattern.accent[j] & bit) != 0);
                    G8RTOS_WriteFIFO()  // update UI
                    }
                }
                if (++seq.sampleCount >= samplesPerStep) {
                    seq.sampleCount = 0;
                    seq.step = (seq.step + 1) & 15;
                }
            }
            // mix, scale and clip voices to 12 bits
            int32_t sample = DAC_MID + ((Synth_Next() * MASTER_VOLUME) >> 8);
            if (sample < 0) sample = 0;
            if (sample > 4095) sample = 4095;
            out[i] = (uint16_t)sample;
        }
        blockReady = true;
    }
}

void UI_Thread(void) {

    while(1) {

    }
}

void Read_Buttons(void) {

    // Initialize / declare any variables here
    uint8_t buttons;
    uint8_t prev_buttons = 0;
    uint8_t pressed;

    while(1) {

        // wait for button semaphore
        G8RTOS_WaitSemaphore(&sem_Buttons);
        sleep(15);

        G8RTOS_WaitSemaphore(&sem_I2CA);
        buttons = MultimodButtons_Get();
        G8RTOS_SignalSemaphore(&sem_I2CA);

        pressed = buttons & (uint8_t)~prev_buttons;
        prev_buttons = buttons;

        // map buttons to their functions
        if (pressed & BTN_SW1){
            // play 
            seq.playing = !seq.playing;
        }
        if (pressed & BTN_SW2){
            // BPM down
            seq.bpm -= 5;
        }     
        if (pressed & BTN_SW3){
            // BPM up
            seq.bpm += 5;
        }     
        if (pressed & BTN_SW4){
            continue;
        }

        GPIOIntClear(GPIO_PORTE_BASE, GPIO_INT_PIN_4);
        GPIOIntEnable(GPIO_PORTE_BASE, GPIO_INT_PIN_4); 
    }
}


/********************************Periodic Threads***********************************/


/*******************************Aperiodic Threads***********************************/

void Button_Handler() {
    GPIOIntDisable(GPIO_PORTE_BASE, GPIO_INT_PIN_4);
    GPIOIntClear(GPIO_PORTE_BASE, GPIO_INT_PIN_4);
    G8RTOS_SignalSemaphore(&sem_Buttons);
}

void Sample_Handler() {
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    MultimodDAC_Write(DAC_OUT_REG, audioBuf[playHalf][playIndex0]);

    if (++playIndex >= BLOCK_SIZE) {
        playIndex = 0;
        playHalf ^= 1;
        if (!blockReady) underruns++;
        blockReady = false;
        G8RTOS_SignalSemaphore(&sem_Mix);
    }
}
