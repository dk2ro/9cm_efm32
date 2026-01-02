#ifndef INC_9CM_EFM32_TMP432_H
#define INC_9CM_EFM32_TMP432_H

enum tmp432_temp {
    TMP432_LOCAL,
    TMP432_REMOTE1,
    TMP432_REMOTE2,
};

void tmp432_init(void);

uint32_t tmp432_get_remote_temp2(void);

void tmp432_get_temperature(enum tmp432_temp, uint32_t *tmp_int, uint32_t *tmp_decimal);

#endif //INC_9CM_EFM32_TMP432_H
