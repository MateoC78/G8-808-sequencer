// multimod_BMI160.c
// Date Created: 2023-07-25
// Date Updated: 2023-07-27
// Defines for BMI160 functions

/************************************Includes***************************************/

#include "../multimod_BMI160.h"

#include <stdint.h>
#include "../multimod_i2c.h"

/************************************Includes***************************************/

/********************************Public Functions***********************************/

// BMI160_Init
// Initializes the BMI160. Currently enables the accelerometer
// in full-power mode.
//0x69
// Return: void
void BMI160_Init() {
    I2C_Init(I2C_A_BASE);

    // Power on accelerometer
    BMI160_WriteRegister(BMI160_CMD_ADDR, 0x11);
    SysCtlDelay(SysCtlClockGet() / 3000);

    BMI160_WriteRegister(BMI160_CMD_ADDR, 0x15);
    SysCtlDelay(SysCtlClockGet() / 3000);

    BMI160_WriteRegister(BMI160_CMD_ADDR, 0x18);
    SysCtlDelay(SysCtlClockGet() / 3000);

    // Configure accelerometer and gyroscope for normal measurements.
    BMI160_WriteRegister(BMI160_ACCCONF_ADDR, 0x28);
    BMI160_WriteRegister(BMI160_ACCRANGE_ADDR, 0x03);

    //enable gyro = 0x10 | 0x08
        // range = 0x04
    BMI160_WriteRegister(BMI160_GYRCONFG_ADDR, 0x10 | 0x08);
    BMI160_WriteRegister(BMI160_GYRRANGE_ADDR, 0x04);
    //enable magnetometer = 0x08
    BMI160_WriteRegister(BMI160_MAGCONF_ADDR, 0x08);

    return;
}

// BMI160_WriteRegister
// Writes to a register address in the BMI160.
// Param uint8_t "addr": Register address
// Param uint8_t "data": data to write
// Return: void
void BMI160_WriteRegister(uint8_t addr, uint8_t data) {
    // write a single register
    // write where?
            // your code here

    // with what? (hint youre writing only 1 data item, but you still need to specify where)
    uint8_t d[2] = {addr, data};
    // write I2C device
    I2C_WriteMultiple(I2C_A_BASE, BMI160_ADDR, d, 2);
}

// BMI160_ReadRegister
// Reads from a register address in the BMI160.
// Param uint8_t "addr": Register address
// Return: void
uint8_t BMI160_ReadRegister(uint8_t addr) {
    // read from which addr?
    I2C_WriteMultiple(I2C_A_BASE, BMI160_ADDR, &addr, 1);
    // your code here
    uint8_t val;
    I2C_ReadMultiple(I2C_A_BASE, BMI160_ADDR, &val, 1);
    // return the data
    return val; // your code here
}

// BMI160_MultiReadRegister
// Uses the BMI160 auto-increment function to read from multiple registers.
// Param uint8_t "addr": beginning register address
// Param uint8_t* "data": pointer to an array to store data in
// Param uint8_t "num_bytes": number of bytes to read
// Return: void
void BMI160_MultiReadRegister(uint8_t addr, uint8_t* data, uint8_t num_bytes) {
    // write to which addr?
    I2C_WriteMultiple(I2C_A_BASE, BMI160_ADDR, &addr, 1);
    
    // read the data from that addr
    I2C_ReadMultiple(I2C_A_BASE, BMI160_ADDR, data, num_bytes);
    return;
}


// BMI160_AccelXGetResult
// Gets the 16-bit x-axis acceleration result.
// Return: uint16_t
int16_t BMI160_AccelXGetResult() {
    uint8_t data[2];
    // if not read, wait till read
    while ((BMI160_GetDataStatus() & BMI160_STATUS_DRDY_ACC) == 0)

    // read data
    data[0] = BMI160_ReadRegister(0x12);
    data[1] = BMI160_ReadRegister(0x13);

    // write data 
    return (int16_t)(((uint16_t)data[1] << 8) | data[0]);
}

// BMI160_AccelYGetResult
// Gets the 16-bit y-axis acceleration result.
// Return: uint16_t
int16_t BMI160_AccelYGetResult() {
    // your code here
    uint8_t data[2];
    // if not read, wait till read
    while ((BMI160_GetDataStatus() & BMI160_STATUS_DRDY_ACC) == 0)

    // read data
    data[0] = BMI160_ReadRegister(0x14);
    data[1] = BMI160_ReadRegister(0x15);

    // write data 
    return (int16_t)(((uint16_t)data[1] << 8) | data[0]);
}

// BMI160_AccelZGetResult
// Gets the 16-bit z-axis acceleration result.
// Return: uint16_t
int16_t BMI160_AccelZGetResult() {
    // your code here
    uint8_t data[2];
    // if not read, wait till read
    while ((BMI160_GetDataStatus() & BMI160_STATUS_DRDY_ACC) == 0)

    // read data
    data[0] = BMI160_ReadRegister(0x16);
    data[1] = BMI160_ReadRegister(0x17);

    // write data 
    return (int16_t)(((uint16_t)data[1] << 8) | data[0]);
}

// BMI160_GyroXGetResult
// Gets the 16-bit x-axis gyroscope result.
// Return: uint16_t
int16_t BMI160_GyroXGetResult() {
    // your code here
    uint8_t data[2];
    // if not read, wait till read
    while ((BMI160_GetDataStatus() & BMI160_STATUS_DRDY_GYR) == 0)

    // read data
    data[0] = BMI160_ReadRegister(0x0C);
    data[1] = BMI160_ReadRegister(0x0D);

    // write data 
    return (int16_t)(((uint16_t)data[1] << 8) | data[0]);
}

// BMI160_GyroYGetResult
// Gets the 16-bit y-axis gyroscope result.
// Return: uint16_t
int16_t BMI160_GyroYGetResult() {
    // your code here
        uint8_t data[2];
    // if not read, wait till read
    while ((BMI160_GetDataStatus() & BMI160_STATUS_DRDY_GYR) == 0)

    // read data
    data[0] = BMI160_ReadRegister(0x0E);
    data[1] = BMI160_ReadRegister(0x0F);

    // write data 
    return (int16_t)(((uint16_t)data[1] << 8) | data[0]);
}

// BMI160_GyroZGetResult
// Gets the 16-bit z-axis gyroscope result.
// Return: uint16_t
int16_t BMI160_GyroZGetResult() {
    // your code here
    uint8_t data[2];
    // if not read, wait till read
    while ((BMI160_GetDataStatus() & BMI160_STATUS_DRDY_GYR) == 0)

    // read data
    data[0] = BMI160_ReadRegister(0x11);
    data[1] = BMI160_ReadRegister(0x10);

    // write data 
    return (int16_t)(((uint16_t)data[1] << 8) | data[0]);
}

// BMI160_MagXGetResult
// Gets the 16-bit x-axis magnetometer result.
// Return: uint16_t
int16_t BMI160_MagXGetResult() {
    // your code here
    uint8_t data[2];
    // if not read, wait till read
    while ((BMI160_GetDataStatus() & BMI160_STATUS_DRDY_MAG) == 0)

    // read data
    data[0] = BMI160_ReadRegister(0x05);
    data[1] = BMI160_ReadRegister(0x04);

    // write data 
    return (int16_t)(((uint16_t)data[1] << 8) | data[0]);
}

// BMI160_MagYGetResult
// Gets the 16-bit y-axis magnetometer result.
// Return: uint16_t
int16_t BMI160_MagYGetResult() {
    // your code here
    uint8_t data[2];
    // if not read, wait till read
    while ((BMI160_GetDataStatus() & BMI160_STATUS_DRDY_MAG) == 0)

    // read data
    data[0] = BMI160_ReadRegister(0x07);
    data[1] = BMI160_ReadRegister(0x06);

    // write data 
    return (int16_t)(((uint16_t)data[1] << 8) | data[0]);
}

// BMI160_MagZGetResult
// Gets the 16-bit z-axis magnetometer result.
// Return: uint16_t
int16_t BMI160_MagZGetResult() {
    // your code here
    uint8_t data[2];
    // if not read, wait till read
    while ((BMI160_GetDataStatus() & BMI160_STATUS_DRDY_MAG) == 0)

    // read data
    data[0] = BMI160_ReadRegister(0x09);
    data[1] = BMI160_ReadRegister(0x08);

    // write data 
    return (int16_t)(((uint16_t)data[1] << 8) | data[0]);
}

// BMI160_AccelXYZGetResult
// Stores the 16-bit XYZ accelerometer results in an array.
// Param uint16_t* "data": pointer to an array of 16-bit data.
// Return: void
void BMI160_AccelXYZGetResult(uint16_t* data) {
    // your code here
    data[0] = BMI160_AccelXGetResult();
    data[1] = BMI160_AccelYGetResult();
    data[2] = BMI160_AccelZGetResult();
}

// BMI160_GyroXYZGetResult
// Stores the 16-bit XYZ gyroscope results in an array.
// Param uint16_t* "data": pointer to an array of 16-bit data.
// Return: void
void BMI160_GyroXYZGetResult(uint16_t* data) {
    // your code here
}

// BMI160_MagXYZGetResult
// Stores the 16-bit XYZ magnetometer results in an array.
// Param uint16_t* "data": pointer to an array of 16-bit data.
// Return: void
void BMI160_MagXYZGetResult(uint16_t* data) {
    // your code here

}

// BMI160_GetDataStatus
// Gets the status register to determine if data is ready to read.
// Return: uint8_t
uint8_t BMI160_GetDataStatus() {
    return BMI160_ReadRegister(BMI160_STATUS_ADDR);
}

/********************************Public Functions***********************************/
