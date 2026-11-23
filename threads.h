// threads.h

#ifndef THREADS_H_
#define THREADS_H_

/************************************Includes***************************************/

#include "./G8RTOS/G8RTOS.h"
#include "./G8-808/synth.h"
#include <inc/hw_memmap.h>
#include <driverlib/sysctl.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/uart.h>

/************************************Includes***************************************/

/*************************************Defines***************************************/

// FIFOs
#define EDIT_FIFO           0
#define JOYSTICK_FIFO       1
#define UI_FIFO             2

// Audio
#define BLOCK_SIZE          64
#define BPM_MIN             60
#define BPM_MAX             200
#define BPM_DEFAULT         120
#define SAMPLE_RATE_HZ      16000
#define DAC_MID             2048
#define MASTER_VOLUME       160

// UI message macros
#define UI_MSG(type, value) (((uint32_t)(type) >> 8) | (uint16_t)(value))
#define UI_MSG_VALUE(msg)   ((uint8_t)(msg))
#define UI_MSG_TYPE(msg)    ((uint8_t)((msg) >> 8))     // gives message types seen below
#define UI_STEP             1
#define UI_STATUS           2
#define UI_PATTERN          3

// Buttons
#define BTN_SW1              0x02
#define BTN_SW2              0x04
#define BTN_SW3              0x08
#define BTN_SW4              0x10

/*************************************Defines***************************************/

/***********************************Semaphores**************************************/

semaphore_t sem_I2CA;
semaphore_t sem_SPIA;
semaphore_t sem_Buttons;
semaphore_t sem_UART;
semaphore_t sem_Mix;
semaphore_t sem_Joystick;

/***********************************Semaphores**************************************/

/***********************************Structures**************************************/
// Pattern: one 16 bit number per track, bit n = step n
typedef struct {
    uint16_t steps[NUM_TRACKS];
    uint16_t accent[NUM_TRACKS];
} Pattern;

// Sequencer clock: counts samples, no jitter 
typedef struct {
    volatile bool playing;
    volatile uint16_t bpm;
    uint32_t sampleCount;
    uint8_t step;
} Sequencer;

/***********************************Structures**************************************/


/*******************************Background Threads**********************************/

void Idle_Thread(void);
void Mix_Thread(void);
void UI_Thread(void);
void Read_Buttons(void);

/*******************************Background Threads**********************************/

/********************************Periodic Threads***********************************/

/********************************Periodic Threads***********************************/

/*******************************Aperiodic Threads***********************************/

void Button_Handler(void);
void Sample_Handler(void);

/*******************************Aperiodic Threads***********************************/


#endif /* THREADS_H_ */

