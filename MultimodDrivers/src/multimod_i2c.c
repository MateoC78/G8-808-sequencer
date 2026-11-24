// multimod_i2c.c
// Date Created: 2023-07-25
// Date Updated: 2023-07-27
// Defines for I2C functions

/************************************Includes***************************************/

#include "../multimod_i2c.h"

#include <driverlib/gpio.h>
#include <driverlib/sysctl.h>
#include <driverlib/pin_map.h>

#include <inc/tm4c123gh6pm.h>
#include <inc/hw_i2c.h>

/************************************Includes***************************************/

/********************************Public Functions***********************************/

// I2C_Init
// Initializes specified I2C module
// Param uint32_t "mod": base address of module
// Return: void
void I2C_Init(uint32_t mod) {
    if (mod == I2C_A_BASE){
        // which I2C module?
        SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
        SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C1);

        // does it require GPIO pins? if so, how do we configure them?
        GPIOPinConfigure(GPIO_PA6_I2C1SCL);
        GPIOPinConfigure(GPIO_PA7_I2C1SDA);

        GPIOPinTypeI2CSCL(I2C_A_GPIO_BASE, I2C_A_PIN_SCL);
        GPIOPinTypeI2C(I2C_A_GPIO_BASE, I2C_A_PIN_SDA);

        // you get this for free
        I2CMasterInitExpClk(I2C_A_BASE, SysCtlClockGet(), false);
    }

    if (mod == I2C_B_BASE){
        // which I2C module?
        SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
        SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);

        // does it require GPIO pins? if so, how do we configure them?
        GPIOPinConfigure(GPIO_PB2_I2C0SCL);
        GPIOPinConfigure(GPIO_PB3_I2C0SDA);

        GPIOPinTypeI2CSCL(I2C_B_GPIO_BASE, I2C_B_PIN_SCL);
        GPIOPinTypeI2C(I2C_B_GPIO_BASE, I2C_B_PIN_SDA);

        // you get this for free
        I2CMasterInitExpClk(I2C_B_BASE, SysCtlClockGet(), false);
    }
}

// I2C_WriteSingle
// Writes a single byte to an address.
// Param uint32_t "mod": base address of module
// Param uint8_t "addr": address to device
// Param uint8_t "byte": byte to send
// Return: void
void I2C_WriteSingle(uint32_t mod, uint8_t addr, uint8_t byte) {
    // which module and where to?
    I2CMasterSlaveAddrSet(mod, addr, false);

    // what data?
    I2CMasterDataPut(mod, byte);

    // what mode?
    I2CMasterControl(mod, I2C_MASTER_CMD_SINGLE_SEND);

    // should we wait till finished?
    while(I2CMasterBusy(mod)){}

    return;
}

// I2C_ReadSingle
// Reads a single byte from address.
// Param uint32_t "mod": base address of module
// Param uint8_t "addr": address to device
// Return: uint8_t
uint8_t I2C_ReadSingle(uint32_t mod, uint8_t addr) {
    // which module and where to? 
    I2CMasterSlaveAddrSet(mod, addr, true);

    // what data?   NONE

    // what mode?
    I2CMasterControl(mod, I2C_MASTER_CMD_SINGLE_RECEIVE);

    // should be wait till finished?
    while(I2CMasterBusy(mod)){}

    // return data from module
    return (I2CMasterDataGet(mod));
}

// I2C_WriteMultiple
// Write multiple bytes to a device.
// Param uint32_t "mod": base address of module
// Param uint8_t "addr": address to device
// Param uint8_t* "data": pointer to an array of bytes
// Param uint8_t "num_bytes": number of bytes to transmit
// Return: void
void I2C_WriteMultiple(uint32_t mod, uint8_t addr, uint8_t* data, uint8_t num_bytes) {
    uint8_t i;
    // which module and where to? 
    I2CMasterSlaveAddrSet(mod, addr, false);

    // what data?
    I2CMasterDataPut(mod, data[0]);

    // what mode?
    I2CMasterControl(mod, (num_bytes == 1)
                                ? I2C_MASTER_CMD_SINGLE_SEND
                                : I2C_MASTER_CMD_BURST_SEND_START);
    // should be wait till finished?
    while(I2CMasterBusy(mod)){}
    // hint: how many times do we need to write? 
    // hint 2: how do we tell the module we're done sending data? 
    for (i = 1; i < num_bytes; i++){
        I2CMasterDataPut(mod, data[i]);
        I2CMasterControl(mod, (i == num_bytes - 1) 
                                    ? I2C_MASTER_CMD_BURST_SEND_FINISH 
                                    : I2C_MASTER_CMD_BURST_SEND_CONT);
        while(I2CMasterBusy(mod)){}
    }

    return;
}

// I2C_ReadMultiple
// Read multiple bytes from a device.
// Param uint32_t "mod": base address of module
// Param uint8_t "addr": address to device
// Param uint8_t* "data": pointer to an array of bytes
// Param uint8_t "num_bytes": number of bytes to read
// Return: void
void I2C_ReadMultiple(uint32_t mod, uint8_t addr, uint8_t* data, uint8_t num_bytes) {
    uint8_t i;
    // which module and where to? 
    I2CMasterSlaveAddrSet(mod, addr, true);

    // what data?

    // what mode?
    I2CMasterControl(mod, (num_bytes == 1)
                                ? I2C_MASTER_CMD_SINGLE_RECEIVE
                                : I2C_MASTER_CMD_BURST_RECEIVE_START);

    // should be wait till finished?
    while (I2CMasterBusy(mod)) {}
    data[0] = I2CMasterDataGet(mod);

    // hint: how many times do we need to read?

    // hint 2: how do we tell the module we're done receiving data? 

    for (i = 1; i < num_bytes; i++){
        I2CMasterControl(mod, (i == num_bytes - 1) 
                                    ? I2C_MASTER_CMD_BURST_RECEIVE_FINISH 
                                    : I2C_MASTER_CMD_BURST_RECEIVE_CONT);
        while(I2CMasterBusy(mod)){}
        data[i] = I2CMasterDataGet(mod);
    }

    return;
}

/********************************Public Functions***********************************/

