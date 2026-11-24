// threads.c
// Date Created: 2026-11-16
// Defines for thread functions.

/************************************Includes***************************************/

#include "./threads.h"

#include "./MultimodDrivers/multimod.h"
#include "./G8-808/ui.h"
#include "./G8-808/audio.h"

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

static const uint16_t demoKick[] = {0x1111, 0x0509, 0x5191, 0x9249};
static const uint16_t demoAccent[] = {0x0001, 0x0101, 0x0101, 0x0001};
#define NUM_DEMOS (sizeof(demoKick) / sizeof(demoKick[0]))

/*********************************Global Variables**********************************/

/********************************Public Functions***********************************/
void seq_init(void) {
    for (int i = 0; i < BLOCK_SIZE; i++) {
		audioBuf[0][i] = DAC_MID;
		audioBuf[1][i] = DAC_MID;
	}

	for (int i = 0; i < NUM_TRACKS; i++) {
        pattern.steps[i] = 0;
        pattern.accent[i] = 0;
    }
    pattern.steps[TRACK_KICK]  = demoKick[0];
    pattern.accent[TRACK_KICK] = demoAccent[0];

    seq.playing = true;
    seq.bpm = BPM_DEFAULT;
    seq.sampleCount = 0;
    seq.step = 0;
}

/*************************************Threads***************************************/

void Idle_Thread(void) {
    while(1);
}

void Mix_Thread(void) {
    bool wasPlaying = false;
    uint8_t i, j;

    Synth_Init();
    Audio_Start();

    while (1) {
        G8RTOS_WaitSemaphore(&sem_Mix);     // a half just finished playing

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
                    for (j = 0; j < NUM_TRACKS; j++) {
                        if (pattern.steps[j] & bit) Synth_Trigger(j, (pattern.accent[j] & bit) != 0);
                    }
                    G8RTOS_WriteFIFO(UI_FIFO, UI_MSG(UI_STEP, seq.step));
                }
                if (++seq.sampleCount >= samplesPerStep) {
                    seq.sampleCount = 0;
                    seq.step = (seq.step + 1) & 15;
                }
            }
            // mix, scale and clip voices to 12 bits
            static uint16_t testCount = 0;
            int32_t sample = ((testCount++ >> 3) & 1) ? DAC_MID + 800 : DAC_MID - 800;
            //int32_t sample = DAC_MID + ((Synth_Next() * MASTER_VOLUME) >> 8);
            if (sample < 0) sample = 0;
            if (sample > 4095) sample = 4095;
            out[i] = (uint16_t)sample;
        }
        blockReady = true;
    }
}

void UI_Thread(void) {
    uint32_t msg;
    uint32_t shownUnderruns = 0;
    uint8_t t;

    // initial screen draw
    G8RTOS_WaitSemaphore(&sem_SPIA);
    UI_DrawScreen();
    for (t = 0; t < NUM_TRACKS; t++) UI_DrawTrack(t, pattern.steps[t], pattern.accent[t]);
    UI_DrawStatus(seq.playing, seq.bpm, underruns);
    G8RTOS_SignalSemaphore(&sem_SPIA);

    while(1) {
        msg = (uint32_t)G8RTOS_ReadFIFO(UI_FIFO);

        G8RTOS_WaitSemaphore(&sem_SPIA);
        // draw based on message type
        switch (UI_MSG_TYPE(msg)) {
            case UI_STEP:
                UI_DrawPlayhead((int8_t)UI_MSG_VALUE(msg));
                break;
            case UI_STATUS:
                UI_DrawStatus(seq.playing, seq.bpm, underruns);
                if (!seq.playing) UI_DrawPlayhead(-1);
                break;
            case UI_PATTERN:
                break;
        }

        if (underruns != shownUnderruns) {
            shownUnderruns = underruns;
            UI_DrawStatus(seq.playing, seq.bpm, shownUnderruns);
        }
        G8RTOS_SignalSemaphore(&sem_SPIA);
    }
}

void Read_Buttons(void) {

    // Initialize / declare any variables here
    uint8_t buttons;
    uint8_t prev_buttons = 0;
    uint8_t pressed;
    uint8_t demo = 0;

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
            G8RTOS_WriteFIFO(UI_FIFO, UI_MSG(UI_STATUS, 0));
        }
        if ((pressed & BTN_SW2) && seq.bpm > BPM_MIN){
            // BPM down
            seq.bpm -= 5;
            G8RTOS_WriteFIFO(UI_FIFO, UI_MSG(UI_STATUS, 0));
        }     
        if ((pressed & BTN_SW3) && seq.bpm < BPM_MAX){
            // BPM up
            seq.bpm += 5;
            G8RTOS_WriteFIFO(UI_FIFO, UI_MSG(UI_STATUS, 0));
        }     
        if (pressed & BTN_SW4){
            demo = (demo + 1) % NUM_DEMOS;
            pattern.steps[TRACK_KICK] = demoKick[demo];
            pattern.accent[TRACK_KICK] = demoAccent[demo];
            G8RTOS_WriteFIFO(UI_FIFO, UI_MSG(UI_PATTERN, 0));
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

    Audio_DACWrite(audioBuf[playHalf][playIndex]);

    if (++playIndex >= BLOCK_SIZE) {
        playIndex = 0;
        playHalf ^= 1;
        if (!blockReady) underruns++;
        blockReady = false;
        G8RTOS_SignalSemaphore(&sem_Mix);
    }
}
