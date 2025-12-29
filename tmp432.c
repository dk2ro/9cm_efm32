//
// Created by robert on 28.12.25.
//

#include "i2c.h"
#include "tmp432.h"

#define TMP432_I2C_ADDR 0x4C

enum tmp432_read_register {
    R_LOCAL_TEMP_H = 0x0,
    R_REMOTE_TEMP1_H = 0x1,
    R_STATUS = 0x2,
    R_CONFIG1 = 0x3,
    R_CONVERSION_RATE = 0x4,
    R_LOCAL_TEMP_HIGH_LIMIT_H = 0x5,
    R_LOCAL_TEMP_LOW_LIMIT_H = 0x6,
    R_REMOTE_TEMP1_HIGH_LIMIT_H = 0x7,
    R_REMOTE_TEMP1_LOW_LIMIT_H = 0x8,
    R_REMOTE_TEMP1_L = 0x10,
    R_REMOTE_TEMP1_HIGH_LIMIT_L = 0x13,
    R_REMOTE_TEMP1_LOW_LIMIT_L = 0x14,
    R_REMOTE_TEMP2_HIGH_LIMIT_H = 0x15,
    R_REMOTE_TEMP2_LOW_LIMIT_H = 0x16,
    R_REMOTE_TEMP2_HIGH_LIMIT_L = 0x17,
    R_REMOTE_TEMP2_LOW_LIMIT_L = 0x18,
    R_REMOTE_THERM_LIMIT = 0x19,
    R_REMOTE2_THERM_LIMIT = 0x1A,
    R_OPEN_STATUS = 0x1B,
    R_CHANNEL_MAS = 0x1F,
    R_LOCAL_THERM_LIMIT = 0x20,
    R_THERM_LIMIT_HYSTERESIS = 0x21,
    R_CONSECUTIVE_ALERT = 0x22,
    R_REMOTE_TEMP2_H = 0x23,
    R_REMOTE_TEMP2_L = 0x24,
    R_CH1_BETA_RANGE_SELECTION = 0x25,
    R_CH2_BETA_RANGE_SELECTION = 0x26,
    R_N_FACT_CORRECTION_REMOTE1 = 0x27,
    R_N_FACT_CORRECTION_REMOTE2 = 0x28,
    R_LOCAL_TEMP_L = 0x29,
    R_HIGH_LIMIT_STATUS = 0x35,
    R_LOW_LIMIT_STATUS = 0x36,
    R_THERM_STATUS = 0x37,
    R_LOCAL_TEMP_HIGH_LIMIT_L = 0x3D,
    R_LOCAL_TEMP_LOW_LIMIT_L = 0x3E,
    R_CONFIG2 = 0x3F,
    R_DEVICE_ID = 0xFD,
    R_MANUFACTURER_ID = 0xFE
};

enum tmp432_write_register {
    W_CONFIG1 = 0x9,
    W_CONVERSION_RATE = 0xA,
    W_LOCAL_TEMP_HIGH_LIMIT_H = 0xB,
    W_LOCAL_TEMP_LOW_LIMIT_H = 0xC,
    W_REMOTE_TEMP1_HIGH_LIMIT_H = 0xD,
    W_REMOTE_TEMP1_LOW_LIMIT_H = 0xE,
    W_ONE_SHOT_START = 0xF,
    W_REMOTE_TEMP1_HIGH_LIMIT_L = 0x13,
    W_REMOTE_TEMP1_LOW_LIMIT_L = 0x14,
    W_REMOTE_TEMP2_HIGH_LIMIT_H = 0x15,
    W_REMOTE_TEMP2_LOW_LIMIT_H = 0x16,
    W_REMOTE_TEMP2_HIGH_LIMIT_L = 0x17,
    W_REMOTE_TEMP2_LOW_LIMIT_L = 0x18,
    W_REMOTE_THERM_LIMIT = 0x19,
    W_REMOTE2_THERM_LIMIT = 0x1A,
    W_OPEN_STATUS = 0x1B,
    W_CHANNEL_MAS = 0x1F,
    W_LOCAL_THERM_LIMIT = 0x20,
    W_THERM_LIMIT_HYSTERESIS = 0x21,
    W_CONSECUTIVE_ALERT = 0x22,
    W_CH1_BETA_RANGE_SELECTION = 0x25,
    W_CH2_BETA_RANGE_SELECTION = 0x26,
    W_N_FACT_CORRECTION_REMOTE1 = 0x27,
    W_N_FACT_CORRECTION_REMOTE2 = 0x28,
    W_HIGH_LIMIT_STATUS = 0x35,
    W_LOW_LIMIT_STATUS = 0x36,
    W_THERM_STATUS = 0x37,
    W_LOCAL_TEMP_HIGH_LIMIT_L = 0x3D,
    W_LOCAL_TEMP_LOW_LIMIT_L = 0x3E,
    W_CONFIG2 = 0x3F,
    W_SOFTWARE_RESET = 0xFC,
};

enum tmp432_config2_register {
    RC = 1 << 2,
    LEN = 1 << 3,
    REN = 1 << 4,
    REN2 = 1 << 5
};

enum tmp432_conversation_rate_register {
    CR_0_0625HZ = 0x0,
    CR_0_125HZ = 0x1,
    CR_0_25HZ = 0x2,
    CR_0_5HZ = 0x3,
    CR_1HZ = 0x4,
    CR_2HZ = 0x5,
    CR_4HZ = 0x6,
    CR_8HZ = 0x7
};
#if  !__SOFTFP__
static float tmp_to_degree(uint8_t tmp_val1, uint8_t tmp_val2) {
    return (float) tmp_val1 + ((float) (tmp_val2 >> 4) / 16.0f);
}
#else
static uint32_t tmp_to_degree(uint8_t tmp_val1, uint8_t tmp_val2) {
    return (uint32_t) tmp_val1 * 10000 + (tmp_val2 >> 4) * 625;
}
#endif


void tmp432_init() {
    i2c_init();
    uint8_t req[2];
    req[0] = W_CONFIG2;
    req[1] = REN2 | LEN | RC;
    i2c_write(TMP432_I2C_ADDR, req, 2); // disable remote channel1
    req[0] = W_CONVERSION_RATE;
    req[1] = CR_1HZ;
    i2c_write(TMP432_I2C_ADDR, req, 2);
}

uint32_t tmp432_get_temperature(enum tmp432_temp temp) {
    uint8_t temp_val[2];

    uint8_t req;
    switch (temp) {
        case TMP432_LOCAL: req = R_LOCAL_TEMP_H;
            break;
        case TMP432_REMOTE1: req = R_REMOTE_TEMP1_H;
            break;
        case TMP432_REMOTE2: req = R_REMOTE_TEMP2_H;
            break;
    }

    // reading two bytes should automatically get high and low byte
    i2c_write_read(TMP432_I2C_ADDR, &req, 1, temp_val, 2);

    return tmp_to_degree(temp_val[0], temp_val[1]);
}
