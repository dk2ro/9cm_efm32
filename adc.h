#ifndef INC_9CM_EFM32_ADC_H
#define INC_9CM_EFM32_ADC_H

// Desired letimer interrupt frequency (in Hz)
#define LE_TIMER_DESIRED  1000
#define ADC_FREQ         16000000
#include <stdint.h>

void adc_init(void);
uint32_t* adc_get_data(uint32_t *len);

#endif //INC_9CM_EFM32_ADC_H