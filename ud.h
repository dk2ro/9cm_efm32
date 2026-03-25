#ifndef INC_9CM_EFM32_UD_H
#define INC_9CM_EFM32_UD_H
#include <stdint.h>

#define UD_FW_MAJ 0
#define UD_FW_MIN 2

uint8_t ud_check_version_and_update(void);
uint32_t ud_get_cal_value(void);
void ud_update_cal_value(uint32_t new_cal_value);

#endif //INC_9CM_EFM32_UD_H