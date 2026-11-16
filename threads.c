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


/*******************************Aperiodic Threads***********************************/

void Button_Handler() {
    GPIOIntDisable(GPIO_PORTE_BASE, GPIO_INT_PIN_4);
    GPIOIntClear(GPIO_PORTE_BASE, GPIO_INT_PIN_4);
    G8RTOS_SignalSemaphore(&sem_Buttons);
}

void Sample_Handler() {
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);


}
