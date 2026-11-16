// G8RTOS_Threads.c
// Date Created: 2023-07-25
// Date Updated: 2023-07-27
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

// Ping Pong buffer
static uint16_t audioBuf[2][BLOCK_SIZE];
static volatile uint8_t playHalf = 0;
static volatile uint8_t playIndex = 0;
static volatile bool blockReady = true;



/*********************************Global Variables**********************************/

/********************************Public Functions***********************************/


/*************************************Threads***************************************/

void Idle_Thread(void) {
    while(1) {}
}

void Mix_Thread(void) {

    uint8_t buttons = 0;
    uint32_t freq_hz;
    TimerDisable(TIMER1_BASE, TIMER_A);
    
    while (1) {
        // wait for button semaphore
        buttons = (uint8_t)G8RTOS_ReadFIFO(BUTTONS_FIFO);

        // debounce buttons

        // Get buttons
        if      (buttons & BTN_SW1) freq_hz = 1000;
        else if (buttons & BTN_SW2) freq_hz = 2000;
        else if (buttons & BTN_SW3) freq_hz = 3000;
        else if (buttons & BTN_SW4) freq_hz = 0; 
        else continue;

        // clear button interrupt
        TimerDisable(TIMER1_BASE, TIMER_A);

        // check which buttons are pressed

            // set DAC output rate to 1000Hz
            //                        2000Hz
            //                        etc.

        if (freq_hz == 0) {
            MultimodDAC_Write(DAC_OUT_REG, 0);          // timer is off, so the ISR isn't using the DAC
        } else {
            TimerLoadSet(TIMER1_BASE, TIMER_A, SysCtlClockGet() / (freq_hz * SIGNAL_STEPS) - 1);

            dac_step = 0;
            TimerEnable(TIMER1_BASE, TIMER_A);
        }
    }
}

void Volume_Thread(void) {

    // define variables
    uint32_t joystick_xy;
    int32_t joystickY;
    int32_t volume;

    while(1) {
        // read joystick values
        joystick_xy = (uint32_t)G8RTOS_ReadFIFO(JOYSTICK_FIFO);

        // normalize the joystick values
        joystickY = (int32_t)(joystick_xy & 0xFFFF) - JOYSTICK_CENTER;
        //if (JOY_INVERT_Y) joystickY_norm = -joystickY_norm;
        if (joystickY > -JOY_DEADZONE && joystickY < JOY_DEADZONE) continue;

        // update volume based on joystickY_norm
        volume = current_volume + joystickY / VOLUME_STEP_DIV;

        // limit volume to 0-4095 (12 bit range)
        if (volume < 0) volume = 0;
        if (volume > VOLUME_MAX) volume = VOLUME_MAX;
        current_volume = (int16_t)volume;
    }
}

void UI_Thread(void) {

    // Initialize / declare any variables here
    uint32_t packed_result;
    uint16_t magnitude_f1, magnitude_f2;
    uint16_t prev_magnitude_f1 = 0, prev_magnitude_f2 = 0;

    while(1) {

        // read display FIFO for updated magnitude ratio (blocks until Mic_Thread writes)
        packed_result = (uint32_t)G8RTOS_ReadFIFO(DISPLAY_FIFO);

        // unpack result values
        magnitude_f1 = (uint16_t)(packed_result >> 16);
        magnitude_f2 = (uint16_t)(packed_result & 0xFFFF);

        // draw the magnitudes on the display

        // limit the magnitude values to the display range (observed max 500)
        if (magnitude_f1 > MAG_DISPLAY_MAX) magnitude_f1 = MAG_DISPLAY_MAX;
        if (magnitude_f2 > MAG_DISPLAY_MAX) magnitude_f2 = MAG_DISPLAY_MAX;

        // scale magnitudes to bar heights in pixels
        magnitude_f1 = (uint16_t)(((uint32_t)magnitude_f1 * BAR_MAX_HEIGHT) / MAG_DISPLAY_MAX);
        magnitude_f2 = (uint16_t)(((uint32_t)magnitude_f2 * BAR_MAX_HEIGHT) / MAG_DISPLAY_MAX);

        // Only redraw the part of each bar that changed to avoid flicker.
        G8RTOS_WaitSemaphore(&sem_SPIA);
        // FREQ_1 bar
        if (magnitude_f1 < prev_magnitude_f1) {
            // clear previous rectangle (the part above the new height)
            ST7789_DrawRectangle(BAR1_X, BAR_Y + magnitude_f1, BAR_WIDTH,
                                 prev_magnitude_f1 - magnitude_f1, ST7789_BLACK);
        } else if (magnitude_f1 > prev_magnitude_f1) {
            // draw new rectangle (extend the bar)
            ST7789_DrawRectangle(BAR1_X, BAR_Y + prev_magnitude_f1, BAR_WIDTH,
                                 magnitude_f1 - prev_magnitude_f1, BAR1_COLOR);
        }

        // FREQ_2 bar
        if (magnitude_f2 < prev_magnitude_f2) {
            ST7789_DrawRectangle(BAR2_X, BAR_Y + magnitude_f2, BAR_WIDTH,
                                 prev_magnitude_f2 - magnitude_f2, ST7789_BLACK);
        } else if (magnitude_f2 > prev_magnitude_f2) {
            ST7789_DrawRectangle(BAR2_X, BAR_Y + prev_magnitude_f2, BAR_WIDTH,
                                 magnitude_f2 - prev_magnitude_f2, BAR2_COLOR);
        }

        G8RTOS_SignalSemaphore(&sem_SPIA);

        // update previous value
        prev_magnitude_f1 = magnitude_f1;
        prev_magnitude_f2 = magnitude_f2;

        sleep(20);

    }
}

void Read_Buttons(void) {

    // Initialize / declare any variables here
    uint8_t buttons;
    uint8_t prev_buttons = 0;
    uint8_t pressed;

    while(1) {

        // wait for button semaphore
        G8RTOS_WaitSemaphore(&sem_PCA9555_Debounce);

        // Get buttons

        // debounce buttons
        sleep(15);

        G8RTOS_WaitSemaphore(&sem_I2CA);
        buttons = MultimodButtons_Get();
        G8RTOS_SignalSemaphore(&sem_I2CA);

        // clear button interrupt

        // update current_buttons value
        pressed = buttons & (uint8_t)~prev_buttons;
        prev_buttons = buttons;

        if (pressed) G8RTOS_WriteFIFO(BUTTONS_FIFO, pressed);

        GPIOIntClear(GPIO_PORTE_BASE, GPIO_INT_PIN_4);
        GPIOIntEnable(GPIO_PORTE_BASE, GPIO_INT_PIN_4); 
    }
}


/********************************Periodic Threads***********************************/

void Update_Volume(void) {

    // read joystick values
    uint32_t joystick_xy = (uint32_t)JOYSTICK_GETXY();

    // push joystick value to fifo
    G8RTOS_WriteFIFO(JOYSTICK_FIFO, joystick_xy);
    return;
}


/*******************************Aperiodic Threads***********************************/

void Button_Handler() {
    GPIOIntDisable(GPIO_PORTE_BASE, GPIO_INT_PIN_4);
    GPIOIntClear(GPIO_PORTE_BASE, GPIO_INT_PIN_4);
    G8RTOS_SignalSemaphore(&sem_Buttons);
}

void Sample_Handler() {
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);


}
