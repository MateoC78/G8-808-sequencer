// multimod_uart.h
// Date Created: 2023-07-25
// Date Updated: 2023-07-27
// UART header file

#ifndef MULTIMOD_UART_H_
#define MULTIMOD_UART_H_

/************************************Includes***************************************/

/************************************Includes***************************************/

/*************************************Defines***************************************/
#define BB_UART_PERIPH   SYSCTL_PERIPH_UART4
#define BB_UART_BASE     UART4_BASE
#define BB_GPIO_PERIPH   SYSCTL_PERIPH_GPIOC
#define BB_GPIO_BASE     GPIO_PORTC_BASE
#define BB_RX_PINCFG     GPIO_PC4_U4RX
#define BB_TX_PINCFG     GPIO_PC5_U4TX
#define BB_PINS          (GPIO_PIN_4 | GPIO_PIN_5)
#define BB_BAUD          115200
/*************************************Defines***************************************/

/******************************Data Type Definitions********************************/
/******************************Data Type Definitions********************************/

/****************************Data Structure Definitions*****************************/
/****************************Data Structure Definitions*****************************/

/***********************************Externs*****************************************/
/***********************************Externs*****************************************/

/********************************Public Variables***********************************/
/********************************Public Variables***********************************/

/********************************Public Functions***********************************/

void UART_Init();
extern void UARTprintf(const char *pcString, ...);
void UART_BeagleBone_Init();

/********************************Public Functions***********************************/

/*******************************Private Variables***********************************/
/*******************************Private Variables***********************************/

/*******************************Private Functions***********************************/
/*******************************Private Functions***********************************/

#endif /* MULTIMOD_UART_H_ */


