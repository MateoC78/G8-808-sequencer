// G8RTOS_Scheduler.c
// Date Created: 2023-07-25
// Date Updated: 2023-07-27
// Defines for scheduler functions

#include "../G8RTOS_Scheduler.h"

/************************************Includes***************************************/

#include <stdint.h>
#include <stdbool.h>

#include "../G8RTOS_CriticalSection.h"

#include <inc/hw_memmap.h>
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_nvic.h"
#include "driverlib/systick.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"

/************************************Includes***************************************/

/********************************Private Variables**********************************/

// Thread Control Blocks - array to hold information for each thread
static tcb_t threadControlBlocks[MAX_THREADS];

//Sleeping Threads - array to hold information for each sleeping thread
static tcb_t* sleepingThreads[MAX_THREADS];

// Thread Stacks - array of arrays for individual stacks of each thread
static uint32_t threadStacks[MAX_THREADS][STACKSIZE];

// Periodic Event Threads - array to hold pertinent information for each thread
static ptcb_t pthreadControlBlocks[MAX_PTHREADS];

// Current Number of Threads currently in the scheduler
static uint32_t NumberOfThreads;

// Current Number of Periodic Threads currently in the scheduler
static uint32_t NumberOfPThreads;

static uint32_t threadCounter = 0;

/********************************Private Variables**********************************/

/*******************************Private Functions***********************************/

// Occurs every 1 ms.
static void InitSysTick(void)
{
    SysTickPeriodSet(SysCtlClockGet() / 1000);
    SysTickIntRegister(SysTick_Handler);
    IntRegister(FAULT_PENDSV, PendSV_Handler);
    SysTickIntEnable();
    SysTickEnable();
}

/*******************************Private Functions***********************************/


/********************************Public Variables***********************************/

uint32_t SystemTime;

tcb_t* CurrentlyRunningThread;
tcb_t* sleepHead;
uint32_t Ibit_State;

/********************************Public Variables***********************************/



/********************************Public Functions***********************************/

void setInitialStack(int i){
    threadControlBlocks[i].stackPointer = &threadStacks[i][STACKSIZE - 16];
    threadStacks[i][STACKSIZE - 1] = 0x01000000;
    threadStacks[i][STACKSIZE - 3] = 0x14141414;
    threadStacks[i][STACKSIZE - 4] = 0x12121212;
    threadStacks[i][STACKSIZE - 5] = 0x03030303;
    threadStacks[i][STACKSIZE - 6] = 0x02020202;
    threadStacks[i][STACKSIZE - 7] = 0x01010101;
    threadStacks[i][STACKSIZE - 8] = 0x00000000;
    threadStacks[i][STACKSIZE - 9] = 0x11111111;
    threadStacks[i][STACKSIZE - 10] = 0x10101010;
    threadStacks[i][STACKSIZE - 11] = 0x09090909;
    threadStacks[i][STACKSIZE - 12] = 0x08080808;
    threadStacks[i][STACKSIZE - 13] = 0x07070707;
    threadStacks[i][STACKSIZE - 14] = 0x06060606;
    threadStacks[i][STACKSIZE - 15] = 0x05050505;
    threadStacks[i][STACKSIZE - 16] = 0x04040404;
}

// SysTick_Handler
// Increments system time, sets PendSV flag to start scheduler.
// Return: void
void SysTick_Handler() {
    SystemTime++;

    ptcb_t* pthread = &pthreadControlBlocks[0];
    for (uint32_t i = 0; i < NumberOfPThreads; i++) {
        if ((int32_t)(SystemTime - pthread->executeTime) >= 0) {
            pthread->executeTime += pthread->period;    // schedule next run 
            pthread->currentTime = SystemTime;          // last time it ran
            pthread->handler();
        }
        pthread = pthread->nextPTCB;
    }
    
    tcb_t* currThread = CurrentlyRunningThread;

    while (sleepHead != 0 && (int32_t)(SystemTime - sleepHead->sleepCount) >= 0) {
        tcb_t* woke = sleepHead;
        sleepHead = woke->sleepNext;
        woke->sleepNext = 0;
        woke->asleep = 0;
    }

    HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV;
}

// G8RTOS_Init
// Initializes the RTOS by initializing system time.
// Return: void
void G8RTOS_Init() {
    uint32_t newVTORTable = 0x20000000;
    uint32_t* newTable = (uint32_t*) newVTORTable;
    uint32_t* oldTable = (uint32_t*) 0;

    uint32_t i;
    for (i = 0; i < 155; i++) {
        newTable[i] = oldTable[i];
    }

    HWREG(NVIC_VTABLE) = newVTORTable;

    SystemTime = 0;
    NumberOfThreads = 0;
    NumberOfPThreads = 0;
}

// G8RTOS_Launch
// Launches the RTOS.
// Return: error codes, 0 if none
int32_t G8RTOS_Launch() {
    InitSysTick();

    CurrentlyRunningThread = &threadControlBlocks[0];
    sleepHead = &sleepingThreads[0];
    IntPrioritySet(FAULT_SYSTICK, 0xE0);
    IntPrioritySet(FAULT_PENDSV, 0xE0);
    G8RTOS_Start(); // call the assembly function
    return 0;
}

// G8RTOS_Scheduler
// Chooses next thread in the TCB. This time uses priority scheduling.
// Return: void
void G8RTOS_Scheduler() {
    // your code
    if (NumberOfThreads == 0) {
        return;
    }

    tcb_t* best = 0;
    tcb_t* iterator = CurrentlyRunningThread->nextTCB;
    for (uint32_t i = 0; i < NumberOfThreads; i++) {
        if (iterator->isAlive && !iterator->asleep && iterator->blocked == 0) {
            if (best == 0 || iterator->priority < best->priority) {
                best = iterator;
            }
        }
        iterator = iterator->nextTCB;
    }
    if (best != 0) {
        CurrentlyRunningThread = best;
    }
}

static tcb_t* findAliveThread(void) {
    if (CurrentlyRunningThread != 0 && CurrentlyRunningThread->isAlive) {
        return CurrentlyRunningThread;
    }
    for (uint32_t i = 0; i < MAX_THREADS; i++) {
        if (threadControlBlocks[i].isAlive) {
            return &threadControlBlocks[i];
        }
    }
    return 0;
}

// G8RTOS_AddThread
// Adds a thread. This is now in a critical section to support dynamic threads.
// It also now should initalize priority and account for live or dead threads.
// Param void* "threadToAdd": pointer to thread function address
// Param uint8_t "priority": priority from [0, 255].
// Param char* "name": character array containing the thread name.
// Return: sched_ErrCode_t
sched_ErrCode_t G8RTOS_AddThread(void (*threadToAdd)(void), uint8_t priority, char *name) {
  // your code
    int32_t status = StartCriticalSection();
    if (NumberOfThreads >= MAX_THREADS) {
        EndCriticalSection(status);
        return THREAD_LIMIT_REACHED;
    }

    int32_t slot = -1;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (!threadControlBlocks[i].isAlive) {
            slot = i;
            break;
        }
    }

    tcb_t* newThread = &threadControlBlocks[slot];
    tcb_t* liveThread = findAliveThread();

    if (liveThread == 0) {
        newThread->nextTCB = newThread;
        newThread->previousTCB = newThread;
    } else {
        newThread->nextTCB = liveThread;
        newThread->previousTCB = liveThread->previousTCB;
        liveThread->previousTCB->nextTCB = newThread;
        liveThread->previousTCB = newThread;
    }
    
    setInitialStack(slot);
    threadStacks[slot][STACKSIZE - 2] = (uint32_t)threadToAdd;
    newThread->priority = priority;
    newThread->asleep = 0;
    newThread->sleepCount = 0;
    newThread->ThreadID = slot;
    uint32_t n = 0;
    if (name != 0) {
        while (n < MAX_NAME_LENGTH - 1 && name[n] != '\0') {
            newThread->threadName[n] = name[n];
            n++;
        }
    }
    newThread->threadName[n] = '\0';
    newThread->blocked = 0;
    newThread->isAlive = 1;
    
    NumberOfThreads++;

    EndCriticalSection(status);
    return NO_ERROR;
        
}

// G8RTOS_Add_APeriodicEvent
// Param void* "AthreadToAdd": pointer to thread function address
// Param uint8_t "priority": Priorit of aperiodic event, [1..6]
// Param int32_t "IRQn": Interrupt request number that references the vector table. [0..155].
// Return: sched_ErrCode_t
sched_ErrCode_t G8RTOS_Add_APeriodicEvent(void (*AthreadToAdd)(void), uint8_t priority, int32_t IRQn) {
    Ibit_State = StartCriticalSection();
    
    if (IRQn < 0 || IRQn >= 155) {
        EndCriticalSection(Ibit_State);
        return IRQn_INVALID;
    }

    if (priority < 1 || priority > 6) {
        EndCriticalSection(Ibit_State);
        return HWI_PRIORITY_INVALID;
    }

    uint32_t* vectors = (uint32_t*) HWREG(NVIC_VTABLE);
    vectors[IRQn] = (uint32_t) AthreadToAdd;

    IntPrioritySet(IRQn, priority << 5);
    IntEnable(IRQn);

    EndCriticalSection(Ibit_State);
    return NO_ERROR;
}

// G8RTOS_Add_PeriodicEvent
// Adds periodic threads to G8RTOS Scheduler
// Function will initialize a periodic event struct to represent event.
// The struct will be added to a linked list of periodic events
// Param void* "PThreadToAdd": void-void function for P thread handler
// Param uint32_t "period": period of P thread to add
// Param uint32_t "execution": When to execute the periodic thread
// Return: sched_ErrCode_t
sched_ErrCode_t G8RTOS_Add_PeriodicEvent(void (*PThreadToAdd)(void), uint32_t period, uint32_t execution) {
    Ibit_State = StartCriticalSection();
    if (NumberOfPThreads >= MAX_PTHREADS) {
        EndCriticalSection(Ibit_State);
        return THREAD_LIMIT_REACHED;
    }

    ptcb_t* newPThread = &pthreadControlBlocks[NumberOfPThreads];

    if (NumberOfPThreads == 0) {
        newPThread->nextPTCB = newPThread;
        newPThread->previousPTCB = newPThread;
    } else {
        ptcb_t* head = &pthreadControlBlocks[0];
        ptcb_t* tail = head->previousPTCB;
        newPThread->nextPTCB = head;
        newPThread->previousPTCB = tail;
        tail->nextPTCB = newPThread;
        head->previousPTCB = newPThread;
    }

    newPThread->handler = PThreadToAdd;
    newPThread->period = period;

    newPThread->executeTime = execution;
    
    NumberOfPThreads++;

    EndCriticalSection(Ibit_State);
    return NO_ERROR;
}

// G8RTOS_KillThread
// Param uint32_t "threadID": ID of thread to kill
// Return: sched_ErrCode_t
sched_ErrCode_t G8RTOS_KillThread(threadID_t threadID) {
    IBit_State = StartCriticalSection();

    if (NumberOfThreads <= 1) {
        EndCriticalSection(IBit_State);
        return CANNOT_KILL_LAST_THREAD;
    }

    tcb_t* threadToKill = 0;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threadControlBlocks[i].isAlive && threadControlBlocks[i].ThreadID == threadID) {
            threadToKill = &threadControlBlocks[i];
            break;
        }
    }

    if (threadToKill == 0) {
        EndCriticalSection(IBit_State);
        return THREAD_DOES_NOT_EXIST;
    }

    threadToKill->isAlive = 0;

    threadToKill->previousTCB->nextTCB = threadToKill->nextTCB;
    threadToKill->nextTCB->previousTCB = threadToKill->previousTCB;

    if (threadToKill->asleep) {
        removeSleepingThread(threadToKill);
    }
    if (threadToKill->blocked) {
        (*threadToKill->blocked)++;
        threadToKill->blocked = 0;
    }
    NumberOfThreads--;
    if (threadToKill == CurrentlyRunningThread) {
        HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV;
    }


    EndCriticalSection(IBit_State);
    return NO_ERROR;
}

// G8RTOS_KillSelf
// Kills currently running thread.
// Return: sched_ErrCode_t
sched_ErrCode_t G8RTOS_KillSelf() {
    // your code
    return G8RTOS_KillThread(CurrentlyRunningThread->ThreadID);
}

// removes from active linked list and adds to sorted sleeping threads LL
void addSleepingThread(tcb_t* thread) {
    if (NumberOfThreads == 0) {
        return;
    }

    thread->asleep = 1;

    tcb_t** link = &sleepHead;

    while (*link != 0 && (int32_t)((*link)->sleepCount - thread->sleepCount) <= 0) {
        link = &(*link)->sleepNext;
    }
    thread->sleepNext = *link;
    *link = thread;
}

void removeSleepingThread(tcb_t* thread) {
    tcb_t** link = &sleepHead;
    
    while (*link != 0) {
        if (*link == thread) {
            *link = thread->sleepNext; 
            thread->sleepNext = 0;
            thread->asleep = 0;
            return 1;
        }
        link = &(*link)->sleepNext;
    }
    return 0;
}

// sleep
// Puts current thread to sleep
// Param uint32_t "durationMS": how many systicks to sleep for
void sleep(uint32_t durationMS) {
    // your stuff goes here
    IBit_State = StartCriticalSection();
    CurrentlyRunningThread->sleepCount = durationMS + SystemTime;
    addSleepingThread(CurrentlyRunningThread);
    EndCriticalSection(IBit_State);

    HWREG(NVIC_INT_CTRL) = NVIC_INT_CTRL_PEND_SV;
    
}

// G8RTOS_GetThreadID
// Gets current thread ID.
// Return: threadID_t
threadID_t G8RTOS_GetThreadID(void) {
    return CurrentlyRunningThread->ThreadID;        //Returns the thread ID
}

// G8RTOS_GetNumberOfThreads
// Gets number of threads.
// Return: uint32_t
uint32_t G8RTOS_GetNumberOfThreads(void) {
    return NumberOfThreads;         //Returns the number of threads
}
/********************************Public Functions***********************************/
