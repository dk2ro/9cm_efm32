//
// Created by robert on 25.12.25.
//

#include "i2c.h"

#include <stddef.h>

#include "em_gpio.h"
#include "em_i2c.h"



void i2c_init() {
    I2C_Init_TypeDef i2cInit = I2C_INIT_DEFAULT;
    // Use ~400khz SCK
    i2cInit.freq = I2C_FREQ_STANDARD_MAX;
    //i2cInit.freq = I2C_FREQ_FAST_MAX;
    // Use 6:3 low high SCK ratio
    //i2cInit.clhr = i2cClockHLRAsymetric;

    GPIO_PinModeSet(I2C_PORT, I2C_SCL_PIN, gpioModeWiredAndPullUpFilter, 1);
    GPIO_PinModeSet(I2C_PORT, I2C_SDA_PIN, gpioModeWiredAndPullUpFilter, 1);

    // Enable I2C SDA and SCL pins, see the readme or the device datasheet for I2C pin mappings
    I2C0->ROUTEPEN = I2C_ROUTEPEN_SDAPEN | I2C_ROUTEPEN_SCLPEN;
    I2C0->ROUTELOC0 = (I2C0->ROUTELOC0 & (~_I2C_ROUTELOC0_SDALOC_MASK)) | I2C_ROUTELOC0_SDALOC;
    I2C0->ROUTELOC0 = (I2C0->ROUTELOC0 & (~_I2C_ROUTELOC0_SCLLOC_MASK)) | I2C_ROUTELOC0_SCLLOC;

    // Initializing the I2C
    I2C_Init(I2C0, &i2cInit);

}




void i2c_write(uint8_t i2c_addr, uint8_t *req, uint8_t req_len)
{
    // Transfer structure
    I2C_TransferSeq_TypeDef i2cTransfer;
    I2C_TransferReturn_TypeDef result;

    // Initializing I2C transfer
    i2cTransfer.addr          = i2c_addr<<1;
    i2cTransfer.flags         = I2C_FLAG_WRITE;
    i2cTransfer.buf[0].data   = req;
    i2cTransfer.buf[0].len    = req_len;
    i2cTransfer.buf[1].data   = NULL;
    i2cTransfer.buf[1].len    = 0;
    result = I2C_TransferInit(I2C0, &i2cTransfer);

    // Sending data
    while (result == i2cTransferInProgress)
    {
        result = I2C_Transfer(I2C0);
    }

}

void i2c_write_read(uint8_t i2c_addr, uint8_t *req, uint8_t req_len, uint8_t *data, uint8_t data_len)
{
    // Transfer structure
    I2C_TransferSeq_TypeDef i2cTransfer;
    I2C_TransferReturn_TypeDef result;


    // Initializing I2C transfer
    i2cTransfer.addr          = i2c_addr<<1;
    i2cTransfer.flags         = I2C_FLAG_WRITE_READ;
    i2cTransfer.buf[0].data   = req;
    i2cTransfer.buf[0].len    = req_len;
    i2cTransfer.buf[1].data   = data;
    i2cTransfer.buf[1].len    = data_len;
    result = I2C_TransferInit(I2C0, &i2cTransfer);

    // Sending data
    while (result == i2cTransferInProgress)
    {
        result = I2C_Transfer(I2C0);
    }

}
