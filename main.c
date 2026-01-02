#define LED_PIN  8
#define LED_PORT gpioPortC

#define PTT_PIN 5
#define PTT_PORT gpioPortA

#define PA_EMERGENCY_OFF_PORT gpioPortA
#define PA_EMERGENCY_OFF_PIN 4

#define INA302_PORT gpioPortD
#define INA302_ALERT1_PIN 12
#define INA302_ALERT2_PIN 11

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_vdac.h"

#include "tmp432.h"
#include "uart.h"
#include "adc.h"


volatile uint8_t e_stop = 0;
volatile uint32_t msTicks; /* counts 1ms timeTicks */

void SysTick_Handler(void) {
    msTicks++; /* increment counter necessary in Delay()*/
}


void Delay(uint32_t dlyTicks) {
    uint32_t curTicks;

    curTicks = msTicks;
    while ((msTicks - curTicks) < dlyTicks);
}


void GPIO_EVEN_IRQHandler(void) {
    // Clear all even pin interrupt flags
    GPIO_IntClear(0x5555);

    GPIO_PinOutSet(PA_EMERGENCY_OFF_PORT, PA_EMERGENCY_OFF_PIN);
    VDAC_ChannelOutputSet(VDAC0, 0, 0);

    // Toggle LED0
    e_stop = 1;
}

void get_min_max_avg(uint32_t *buff, uint32_t len, uint32_t *min, uint32_t *max, uint32_t *avg) {
    uint32_t lmin = UINT32_MAX;
    uint32_t lmax = 0;
    uint32_t avg_sum = 0;

    for (uint32_t i = 0; i < len; i++) {
        if (buff[i] > lmax) lmax = buff[i];
        if (buff[i] < lmin) lmin = buff[i];
        avg_sum += buff[i];
    }

    *min = lmin * 1250 / 8192;
    *max = lmax * 1250 / 8192;
    *avg = avg_sum * 1250 / 8192 / len;
}


int main(void) {
    CHIP_Init();

    CMU_ClockEnable(cmuClock_GPIO, true);
    CMU_ClockEnable(cmuClock_VDAC0, true);
    CMU_ClockEnable(cmuClock_HFPER, true);
    CMU_ClockEnable(cmuClock_USART0, true);
    CMU_ClockEnable(cmuClock_I2C0, true);

    /* Setup SysTick Timer for 1 msec interrupts  */
    if (SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000)) while (1);

    /* Initialize LED driver */
    GPIO_PinModeSet(LED_PORT, LED_PIN, gpioModePushPull, 0);

    GPIO_PinOutSet(LED_PORT, LED_PIN);

    GPIO_PinModeSet(PTT_PORT, PTT_PIN, gpioModeInput, 0);

    GPIO_PinModeSet(INA302_PORT, INA302_ALERT1_PIN, gpioModeInputPull, 1);
    GPIO_PinModeSet(INA302_PORT, INA302_ALERT2_PIN, gpioModeInputPull, 1);
    GPIO_PinModeSet(PA_EMERGENCY_OFF_PORT, PA_EMERGENCY_OFF_PIN, gpioModePushPull, 0);

    NVIC_EnableIRQ(GPIO_EVEN_IRQn);
    NVIC_SetPriority(GPIO_EVEN_IRQn, 0); // 0 is highest priority (default is 0, so this call is just for documentation)
    GPIO_ExtIntConfig(INA302_PORT, INA302_ALERT1_PIN,INA302_ALERT1_PIN, 0, 1, true);

    VDAC_Init_TypeDef vdacInit = VDAC_INIT_DEFAULT;
    VDAC_InitChannel_TypeDef vdacChInit = VDAC_INITCHANNEL_DEFAULT;
    // Set prescaler to get 1 MHz VDAC clock frequency.
    vdacInit.prescaler = VDAC_PrescaleCalc(1000000, true, 0);
    vdacInit.reference = vdacRefAvdd;
    VDAC_Init(VDAC0, &vdacInit);
    vdacChInit.enable = true;
    VDAC_InitChannel(VDAC0, &vdacChInit, 0);

    VDAC_ChannelOutputSet(VDAC0, 0, 0);

    adc_init();
    uart_init();
    tmp432_init();

    while (1) {
        if (e_stop) goto E_STOP;


        uint8_t ptt = GPIO_PinInGet(PTT_PORT,PTT_PIN);
        GPIO_PortOutSetVal(LED_PORT, ptt << LED_PIN, 1 << LED_PIN);
        if (ptt) {
            VDAC_ChannelOutputSet(VDAC0, 0, 2810);
        } else {
            VDAC_ChannelOutputSet(VDAC0, 0, 0);
        }

        uint32_t data_len;
        uint32_t *adc_buff = adc_get_data(&data_len);
        if (data_len) {
            uint32_t tmp_l_int, tmp_r_int, tmp_l_decimal, tmp_r_decimal;
            tmp432_get_temperature(TMP432_LOCAL, &tmp_l_int, &tmp_l_decimal);
            tmp432_get_temperature(TMP432_REMOTE2, &tmp_r_int, &tmp_r_decimal);


            uint32_t i_min, i_max, i_avg;
            get_min_max_avg(adc_buff, data_len, &i_min, &i_max, &i_avg);

            char buff[80];

            sprintf(buff, "i_avg = %lu mA (min %lu, max %lu), temp_r = %lu.%04lu, temp_l = %lu.%04lu\r\n",
                    i_avg, i_min, i_max, tmp_r_int, tmp_r_decimal, tmp_l_int, tmp_l_decimal);
            uart_tx(buff);
        }
    }

E_STOP:
    uart_tx("EMERGENCY STOP! Overcurrent\r\n");
    while (1) {
        GPIO_PinOutToggle(LED_PORT, LED_PIN);
        Delay(100);
    }
}
