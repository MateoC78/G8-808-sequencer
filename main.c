/************************************Includes***************************************/

#include "G8RTOS/G8RTOS.h"
#include "./MultimodDrivers/multimod.h"
#include <stdint.h>
#include <stdbool.h>

#include "./threads.h"
#include <driverlib/sysctl.h>
#include "driverlib/fpu.h"

/************************************Includes***************************************/

/*************************************Defines***************************************/
/*************************************Defines***************************************/

/********************************Public Variables***********************************/
/********************************Public Variables***********************************/


/************************************MAIN*******************************************/

int main(void)
{
    // Sets clock speed to 80 MHz. You'll need it!
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    // Enable the FPU, but turn off automatic FP context stacking. With stacking
    // on, a thread that uses floats (Mic_Thread/goertzel) gets a 26-word
    // exception frame, which PendSV_Handler doesn't handle -> hard fault.
    // Keep float math in Mic_Thread only, since FP registers aren't saved per thread.
    FPUEnable();
    FPUStackingDisable();

    // you might want a delay here (~10 ms) to make sure the display has powered up
    SysCtlDelay(SysCtlClockGet() / 300);   // ~10 ms (SysCtlDelay = 3 cycles/loop)

    // initialize the G8RTOS framework
    Multimod_Init();
    G8RTOS_Init();

    // clear the display before the bars are drawn
    ST7789_Fill(ST7789_BLACK);

    // Add semaphores, threads, FIFOs here
    G8RTOS_InitSemaphore(&sem_UART,1);
    G8RTOS_InitSemaphore(&sem_I2CA,1);
    G8RTOS_InitSemaphore(&sem_SPIA,1);
    G8RTOS_InitSemaphore(&sem_Buttons,0);
    G8RTOS_InitSemaphore(&sem_Joystick,1);
    G8RTOS_InitSemaphore(&sem_Mix,1);


    // FIFOs: one audio FIFO per Goertzel frequency, plus the display FIFO
    G8RTOS_InitFIFO(EDIT_FIFO);
    G8RTOS_InitFIFO(JOYSTICK_FIFO);
    G8RTOS_InitFIFO(UI_FIFO);

    // background threads (lower number = higher priority)
    G8RTOS_AddThread(Idle_Thread,    255, "idle");
    G8RTOS_AddThread(UI_Thread,   2, "display");
    G8RTOS_AddThread(Mix_Thread,   3, "mix");
    G8RTOS_AddThread(Read_Buttons,     3, "buttons");

    // add periodic and aperiodic events here (check multimod_mic.h and multimod_buttons.h for defines)
    G8RTOS_Add_APeriodicEvent(Sample_Handler, 1, DAC_INTERRUPT);
    G8RTOS_Add_APeriodicEvent(Button_Handler, 4, INT_GPIOE);   

    // G8RTOS_Add_PeriodicEvent(Update_Volume, 50, 3);



    G8RTOS_Launch();

    while (1);
}