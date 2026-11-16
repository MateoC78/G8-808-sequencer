// G8RTOS_IPC.c
// Date Created: 2023-07-25
// Date Updated: 2023-07-27
// Defines for FIFO functions for interprocess communication

#include "../G8RTOS_IPC.h"

/************************************Includes***************************************/

#include "../G8RTOS_Semaphores.h"
#include "../G8RTOS_CriticalSection.h"

/************************************Includes***************************************/

/******************************Data Type Definitions********************************/

#define FIFO_SIZE 16
#define MAX_NUMBER_OF_FIFOS 4

/******************************Data Type Definitions********************************/

/****************************Data Structure Definitions*****************************/

typedef struct G8RTOS_FIFO_t {
    uint32_t buffer[FIFO_SIZE];
    uint32_t* head;
    uint32_t* tail;
    uint32_t lostCount;
    semaphore_t currSize;
    semaphore_t mutex;
} G8RTOS_FIFO_t;

/****************************Data Structure Definitions*****************************/

/***********************************Externs*****************************************/
/***********************************Externs*****************************************/

/********************************Public Variables***********************************/

static G8RTOS_FIFO_t FIFOs[MAX_NUMBER_OF_FIFOS];

/********************************Public Variables***********************************/

/********************************Public Functions***********************************/

// G8RTOS_InitFIFO
// Initializes FIFO, points head & tai to relevant buffer
// memory addresses.
// Param "FIFO_index": Index of FIFO block
// Return: int32_t, -1 if error (i.e. FIFO full), 0 if okay
int32_t G8RTOS_InitFIFO(uint32_t FIFO_index) {
    if (FIFO_index >= MAX_NUMBER_OF_FIFOS) {
        return -1;
    }

    G8RTOS_FIFO_t* fifo = &FIFOs[FIFO_index];
    fifo->head = fifo->buffer;
    fifo->tail = fifo->buffer;
    fifo->lostCount = 0;
    G8RTOS_InitSemaphore(&fifo->currSize, 0);
    G8RTOS_InitSemaphore(&fifo->mutex, 1);

    return 0;

}

// G8RTOS_ReadFIFO
// Reads data from head pointer of FIFO.
// Param "FIFO_index": Index of FIFO block
// Return: int32_t, data at head pointer
int32_t G8RTOS_ReadFIFO(uint32_t FIFO_index) {
    if (FIFO_index >= MAX_NUMBER_OF_FIFOS) {
        return -1;
    }

    G8RTOS_FIFO_t* fifo = &FIFOs[FIFO_index];
    
    G8RTOS_WaitSemaphore(&fifo->mutex);
    G8RTOS_WaitSemaphore(&fifo->currSize);

    uint32_t crit = StartCriticalSection();

    int32_t data = *(fifo->head);
    fifo->head++;
    if (fifo->head == fifo->buffer + FIFO_SIZE) {
        fifo->head = fifo->buffer;
    }

    G8RTOS_SignalSemaphore(&fifo->mutex);
    EndCriticalSection(crit);

    return data;
}

// G8RTOS_WriteFIFO
// Writes data to tail of buffer.
// Param "FIFO_index": Index of FIFO block
// Return: int32_t, data at head pointer
int32_t G8RTOS_WriteFIFO(uint32_t FIFO_index, uint32_t data) {
    if (FIFO_index >= MAX_NUMBER_OF_FIFOS) {
        return -1;
    }

    G8RTOS_FIFO_t* fifo = &FIFOs[FIFO_index];

    uint32_t crit = StartCriticalSection();

    *(fifo->tail) = data;
    fifo->tail++;
    if (fifo->tail == fifo->buffer + FIFO_SIZE) {
        fifo->tail = fifo->buffer;
    }

    if (fifo->currSize > FIFO_SIZE-1) {
        fifo->head = fifo->tail;
        fifo->lostCount++;
    }
    else {
        G8RTOS_SignalSemaphore(&fifo->currSize);
    }

    EndCriticalSection(crit);

    return 0;

}

/********************************Public Functions***********************************/
