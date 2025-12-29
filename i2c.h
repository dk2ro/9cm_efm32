//
// Created by robert on 25.12.25.
//
#ifndef INC_9CM_EFM32_I2C_H
#define INC_9CM_EFM32_I2C_H


#define I2C_PORT gpioPortD
#define I2C_SCL_PIN 15
#define I2C_SDA_PIN 13
#define I2C_ROUTELOC0_SCLLOC I2C_ROUTELOC0_SCLLOC_LOC22
#define I2C_ROUTELOC0_SDALOC I2C_ROUTELOC0_SDALOC_LOC21

#include <sys/_stdint.h>


void i2c_init(void);

void i2c_write(uint8_t i2c_addr, uint8_t *req, uint8_t req_len);
void i2c_write_read(uint8_t i2c_addr, uint8_t *req, uint8_t req_len, uint8_t *data, uint8_t data_len);

#endif //INC_9CM_EFM32_I2C_H
