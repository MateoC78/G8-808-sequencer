// multimod_Joystick.h
// Date Created: 2023-07-25
// Date Updated: 2023-07-27
// header file for functions related to the joystick on the multimod board

#ifndef MULTIMOD_JOYSTICK_H_
#define MULTIMOD_JOYSTICK_H_

/************************************Includes***************************************/

#include <stdint.h>
#include <stdbool.h>

/************************************Includes***************************************/

/*************************************Defines***************************************/
#define JOYSTICK_ADC_BASE       ADC0_BASE
#define JOYSTICK_ADC_SEQ        1           // SS1: up to 4 steps, we use 2 (X, Y)
 
#define JOYSTICK_XY_GPIO_BASE   GPIO_PORTE_BASE
#define JOYSTICK_X_PIN          GPIO_PIN_3  // AIN0
#define JOYSTICK_Y_PIN          GPIO_PIN_2  // AIN1
 
#define JOYSTICK_SW_GPIO_BASE   GPIO_PORTD_BASE
#define JOYSTICK_SW_PIN         GPIO_PIN_2
#define JOYSTICK_SW_INT_PIN     GPIO_INT_PIN_2
 
#define JOYSTICK_ADC_MAX        4095        
#define JOYSTICK_CENTER         2048

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

void JOYSTICK_Init();
uint8_t JOYSTICK_IntEnable();
int32_t JOYSTICK_GETXY();
bool JOYSTICK_GetPress();

/********************************Public Functions***********************************/

/*******************************Private Variables***********************************/
/*******************************Private Variables***********************************/

/*******************************Private Functions***********************************/
/*******************************Private Functions***********************************/

#endif /* MULTIMOD_JOYSTICK_H_ */
