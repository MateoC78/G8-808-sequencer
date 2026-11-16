// multimod_OPT3001.c
// Date Created: 2023-07-25
// Date Updated: 2023-07-27
// Defines for OPT3001 functions

/************************************Includes***************************************/

#include "../multimod_OPT3001.h"

#include <stdint.h>
#include "../multimod_i2c.h"

/************************************Includes***************************************/

#define OPT3001_CONFIG_ADDR     0x01

/********************************Public Functions***********************************/

// OPT3001_Init
// Initializes OPT3001, configures it to continuous conversion mode.
//0x47
// Return: void
void OPT3001_Init(void) {
    I2C_Init(I2C_A_BASE);

    // normally would add a software reset here, however,
    // opt3001 does not support software resets.
    OPT3001_SetConfig(0xC610);
    OPT3001_SetLowLimit(0x0, 0x000);
    OPT3001_SetHighLimit(0xB, 0xFFF);

    // Allow time for config register to take effect
    SysCtlDelay(SysCtlClockGet() / 3000);

    return;
}

// OPT3001_WriteRegister
// Writes to a register in the OPT3001.
// Param uint8_t "addr": Register address of the OPT3001.
// Param uint16_t "data": 16-bit data to write to the register.
// Return: void
void OPT3001_WriteRegister(uint8_t addr, uint16_t data) {
    // to where and with what?
    uint8_t bytes[3] = {addr, (data >> 8) & 0xFF, data & 0xFF};
    I2C_WriteMultiple(I2C_A_BASE, OPT3001_ADDR, bytes, 3);

    return;
}

// OPT3001_ReadRegister
// Reads from a register in the OPT3001.
// Param uint8_t "addr": Register address of the OPT3001.
// Return: uint16_t
uint16_t OPT3001_ReadRegister(uint8_t addr) {
    // from where into what?
    uint8_t bytes[2];
    I2C_WriteMultiple(I2C_A_BASE, OPT3001_ADDR, &addr, 1);
    I2C_ReadMultiple(I2C_A_BASE, OPT3001_ADDR, bytes, 2);
    
    return ((bytes[0] << 8) | bytes[1]);
}

// OPT3001_GetResult
// Gets conversion result, calculates byte result based on datasheet
// and configuration settings.
// Return: uint32_t
uint32_t OPT3001_GetResult(void) {
    // wait until ready
    uint16_t result;
    while ((OPT3001_ReadRegister(OPT3001_CONFIG_ADDR) & OPT3001_FLAG_CRF) == 0)
    
    result = OPT3001_ReadRegister(OPT3001_RESULT_ADDR);

    // free
    return LUX((result >> 12) & 0xF, result & 0x0FFF);
}

// OPT3001_SetLowLimit
// Sets the low limit register.
// Param uint16_t "exp": Exponential bound
// Param uint16_t "result": Conversion bound
// Return: void
void OPT3001_SetLowLimit(uint16_t exp, uint16_t result) {
    // free
    OPT3001_WriteRegister(OPT3001_LOWLIMIT_ADDR, (exp << OPT3001_RESULT_E_S | (result & 0xFFF)));

    return;
}

// OPT3001_SetHighLimit
// Sets the high limit register.
// Param uint16_t "exp": Exponential bound
// Param uint16_t "result": Conversion bound
// Return: void
void OPT3001_SetHighLimit(uint16_t exp, uint16_t result) {
    // free
    OPT3001_WriteRegister(OPT3001_HIGHLIMIT_ADDR, (exp << OPT3001_RESULT_E_S | (result & 0xFFF)));

    return;
}

// OPT3001_SetConfig
// Sets the configuration register.
// Param uint16_t "config": configuration bits
// Return: void
void OPT3001_SetConfig(uint16_t config) {
    // where are "configs" normally sent to / stored for a device?
    OPT3001_WriteRegister(OPT3001_CONFIG_ADDR, config);

    return;
}

// OPT3001_GetChipID
// Gets the chip ID of the OPT3001.
// Return: uint16_t
uint16_t OPT3001_GetChipID(void) {
    return OPT3001_ReadRegister(0x47);
    // hint go look in the header files and datasheet for where the device ID is located
}

/********************************Public Functions***********************************/
