//
// Created by robert on 05.01.26.
//

#ifndef INC_9CM_EFM32_VDAC_H
#define INC_9CM_EFM32_VDAC_H
#include <stdint.h>

void vdac_init(void);
void vdac_set_gate_bias(uint32_t gate_bias);

#endif //INC_9CM_EFM32_VDAC_H