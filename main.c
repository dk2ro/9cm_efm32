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
#include <sys/_intsup.h>

#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_vdac.h"

#include "tmp432.h"
#include "uart.h"
#include "adc.h"
#include "ud.h"
#include "vdac.h"


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

void calibration(void) {
    uart_tx("###########################################\r\n");
    uart_tx("#         Calibration Routine             #\r\n");
    uart_tx("###########################################\r\n");
    uart_tx("Ready to start calibration? (y)\r\n");
    char c;


    // check for user input (y or enter key)
    do {
        c = uart_rx();
        if (c && c != 'y' && c != '\r' && c != '\n') {
            return; // user quit
        }
    } while (c == 0);

    uart_tx("Starting calibration\r\n");

    uint8_t ptt = GPIO_PinInGet(PTT_PORT,PTT_PIN);
    if (!ptt) {
        uart_tx("Waiting for TX Pin (5) to become high.\r\n");
        while (!ptt) {
            ptt = GPIO_PinInGet(PTT_PORT,PTT_PIN);
        }
    }

    uint32_t offset = 1000;
    uint32_t cur_val = offset;
    int32_t error = 0;
    int32_t sum_error = 0;

    for (uint8_t k = 0; k < 20; k++) {
        ptt = GPIO_PinInGet(PTT_PORT,PTT_PIN);
        if (!ptt) {
            vdac_set_gate_bias(0);
            uart_tx("Calibration aborted, TX Pin low\r\n");
            return;
        }
        if (e_stop) {
            vdac_set_gate_bias(0);
            uart_tx("Calibration aborted, overcurrent\r\n");
            return;
        }

        vdac_set_gate_bias(cur_val);
        Delay(1000);


        uint32_t data_len;
        uint32_t *adc_buff;
        do {
            adc_buff = adc_get_data(&data_len);
        } while (!data_len);

        uint32_t sum = 0;
        for (uint32_t i = 0; i < data_len; i++) {
            sum += adc_buff[i];
        }

        uint32_t target = 80 * 4096 * 2 * data_len / 1250;
        error = (int32_t) target - (int32_t) sum;

        char buff[80];
        sprintf(buff, "Iteration %d , val = %lu:, target = %lu, sum = %lu, error = %ld \r\n", k + 1, cur_val, target,
                sum, error);
        uart_tx(buff);

        int32_t p_fact = 1;
        int32_t i_fact = 10;

        int32_t p_term = p_fact * error;

        sum_error += error;
        int32_t i_term = i_fact * sum_error;

        cur_val = offset + (p_term + i_term) / 8192;

        if (cur_val > 4000 || cur_val < 1000) {
            vdac_set_gate_bias(0);
            uart_tx("Calibration aborted, set value out of range\r\n");
            return;
        }
    }

    vdac_set_gate_bias(0);

    if (error < -1000 || error > 1000) {
        uart_tx("Calibration failed, error still too high\r\n");
        return;
    }

    uart_tx("Calibration successful. Writing new value to flash.\r\n");
    ud_update_cal_value(cur_val);
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

    adc_init();
    uart_init();
    tmp432_init();
    vdac_init();
    ud_check_version();

    while (1) {
        if (e_stop) goto E_STOP;

        uint8_t ptt = GPIO_PinInGet(PTT_PORT,PTT_PIN);
        GPIO_PortOutSetVal(LED_PORT, ptt << LED_PIN, 1 << LED_PIN);
        if (ptt) {
            vdac_set_gate_bias(ud_get_cal_value());
        } else {
            vdac_set_gate_bias(0);
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

        char c = uart_rx();
        if (c == 'c') {
            calibration();
        }
    }

E_STOP:
    uart_tx("EMERGENCY STOP! Overcurrent\r\n");
    while (1) {
        GPIO_PinOutToggle(LED_PORT, LED_PIN);
        Delay(100);
    }
}
