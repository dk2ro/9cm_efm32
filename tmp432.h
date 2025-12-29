//
// Created by robert on 28.12.25.
//

#ifndef INC_9CM_EFM32_TMP432_H
#define INC_9CM_EFM32_TMP432_H

enum tmp432_temp {
    TMP432_LOCAL,
    TMP432_REMOTE1,
    TMP432_REMOTE2,
};

void tmp432_init(void);

uint32_t tmp432_get_remote_temp2(void);

uint32_t tmp432_get_temperature(enum tmp432_temp);

#endif //INC_9CM_EFM32_TMP432_H
