#define LED_PIN  8
#define LED_PORT gpioPortC

#define PTT_PIN 5
#define PTT_PORT gpioPortA

#define TX_CONSENT_PIN 15
#define TX_CONSENT_PORT gpioPortB

#define PTT_OUT_PIN 11
#define PTT_OUT_PORT gpioPortC


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
#include "state_machine.h"
#include "ud.h"
#include "vdac.h"


volatile uint8_t e_stop = 0;
volatile uint32_t msTicks; /* counts 1ms timeTicks */

struct {
    volatile uint8_t enabled;
    volatile uint32_t interval;
    volatile uint32_t val;
} blink = {
    .enabled = false,
};

void SysTick_Handler(void) {
    msTicks++; /* increment counter necessary in Delay()*/

    if (blink.enabled && blink.val == msTicks) {
        GPIO_PinOutToggle(LED_PORT, LED_PIN);
        blink.val += blink.interval;
    }
}


void Delay(uint32_t dlyTicks) {
    uint32_t curTicks;

    curTicks = msTicks;
    while ((msTicks - curTicks) < dlyTicks);
}

void blink_start(uint8_t freq) {
    blink.interval = 1000 / (freq * 2);
    blink.enabled = true;
    blink.val = msTicks + blink.interval;
}

void blink_stop(void) {
    blink.enabled = false;
    GPIO_PinOutClear(LED_PORT, LED_PIN);
}

void m(char c) {
    uint8_t a[] = {
        0xa0, 0xa1, 0xa3, 0xa7, 0xaf, 0xbf, 0xbe, 0xbc, 0xb8, 0xb0,
        0x41, 0x8e, 0x8a, 0x66, 0x21, 0x8b, 0x64, 0x8f, 0x43, 0x81, 0x62, 0x8d, 0x40, 0x42, 0x60, 0x89, 0x84, 0x65,
        0x67, 0x20, 0x63, 0x87, 0x61, 0x86, 0x82, 0x8c
    };
    uint8_t ch = c >= 'a' ? a[c - 'a' + 10] : a[(uint8_t) c];

    uint8_t len = (ch & 0xe0u) >> 5u;
    while (len--) {
        GPIO_PinOutSet(LED_PORT, LED_PIN);
        Delay((3 - (ch & 1u) * 2) * 200);
        GPIO_PinOutClear(LED_PORT, LED_PIN);
        Delay(200);
        ch >>= 1u;
    }
    Delay(400);
}

void GPIO_EVEN_IRQHandler(void) {
    // Clear all even pin interrupt flags
    GPIO_IntClear(0x5555);

    VDAC_ChannelOutputSet(VDAC0, 0, 0);
    GPIO_PinOutSet(PA_EMERGENCY_OFF_PORT, PA_EMERGENCY_OFF_PIN);
    GPIO_PinOutClear(PTT_OUT_PORT, PTT_OUT_PIN);

    e_stop = 1;
}

uint8_t calibration(void) {
    uart_tx("###########################################\r\n");
    uart_tx("#         Calibration Routine             #\r\n");
    uart_tx("###########################################\r\n");

    uart_tx("Starting calibration\r\n");
    m('c');
    Delay(1000);
    blink_start(1);

    uint8_t ptt = GPIO_PinInGet(PTT_PORT,PTT_PIN);
    if (!ptt) {
        uart_tx("Waiting for TX Pin (5) to become high.\r\n");
        while (!ptt) {
            ptt = GPIO_PinInGet(PTT_PORT,PTT_PIN);
        }
    }

    blink_start(2);

    uint32_t offset = 1000;
    uint32_t cur_val = offset;
    int32_t error = 0;
    int32_t sum_error = 0;

    for (uint8_t k = 0; k < 20; k++) {
        ptt = GPIO_PinInGet(PTT_PORT,PTT_PIN);
        if (!ptt) {
            vdac_set_gate_bias(0);
            uart_tx("Calibration aborted, TX Pin low\r\n");
            return 1;
        }
        if (e_stop) {
            vdac_set_gate_bias(0);
            uart_tx("Calibration aborted, overcurrent\r\n");
            return 2;
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

        uint32_t target = 80 * 4096 * 2 * data_len / 2500;
        error = (int32_t) target - (int32_t) sum;

        uint32_t mv = sum * 3125 / (1024 * data_len); // this might overflow (value is only for display)

        char buff[80];
        sprintf(buff, "Iteration %d, DAC val = %lu, target = %lu, measured = %lu (%lu.%lu mA), error = %ld \r\n", k + 1,
                cur_val, target,
                sum, mv / 10, mv % 10, error);
        uart_tx(buff);

        int32_t p_fact = 2;
        int32_t i_fact = 10;

        int32_t p_term = p_fact * error;

        sum_error += error;
        int32_t i_term = i_fact * sum_error;

        cur_val = offset + (p_term + i_term) / 8192;

        if (cur_val > 4000 || cur_val < 1000) {
            vdac_set_gate_bias(0);
            uart_tx("Calibration aborted, set value out of range\r\n");
            return 3;
        }
    }

    vdac_set_gate_bias(0);

    if (error < -1000 || error > 1000) {
        uart_tx("Calibration failed, error still too high\r\n");
        return 4;
    }

    uart_tx("Calibration successful. Writing new value to flash.\r\n");
    ud_update_cal_value(cur_val);

    m('r');
    m(cur_val / 1000);
    m(cur_val / 100 % 10);
    m(cur_val / 10 % 10);
    m(cur_val % 10);


    return 0;
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

    // Current Measurement 2 V/A, ADC Ref 2,5V, 12 bit

    *min = lmin * 2500 / 8192;
    *max = lmax * 2500 / 8192;
    *avg = avg_sum * 625 / 2048 / len;
}

void tRX_init(void), tRX_task(void);
void tRX_lnaoff_init(void), tRX_lnaoff_task(void);
void tTX_pre_drive_init(void), tTX_pre_drive_task(void);
void tTX_ampoff_init(void), tTX_ampoff_task(void);
void tTX_init(void), tTX_task(void);


state_t tRX = {
    .init_task = &tRX_init,
    .task = &tRX_task,
    .interrupt_task = &state_machine_state_null
};

state_t tRX_lnaoff = {
    .init_task = &tRX_lnaoff_init,
    .task = &tRX_lnaoff_task,
    .interrupt_task = &state_machine_state_null
};

state_t tTX_pre_drive = {
    .init_task = &tTX_pre_drive_init,
    .task = &tTX_pre_drive_task,
    .interrupt_task = &state_machine_state_null
};

state_t tTX = {
    .init_task = &tTX_init,
    .task = &tTX_task,
    .interrupt_task = &state_machine_state_null
};


state_t tTX_ampoff = {
    .init_task = &tTX_ampoff_init,
    .task = &tTX_ampoff_task,
    .interrupt_task = &state_machine_state_null
};


void tRX_init() {
    uart_tx("STATE RX\r\n");
}

void tRX_task() {
    if (GPIO_PinInGet(PTT_PORT,PTT_PIN)) {
        state_machine_next_state(&tRX_lnaoff);
    }
}


void tRX_lnaoff_init() {
    state_machine_start_timer(100);
    uart_tx("STATE RX (lna off)\r\n");
}

void tRX_lnaoff_task() {
    if (state_machine_interrupt_flag) {
        if (GPIO_PinInGet(PTT_PORT,PTT_PIN)) {
            state_machine_next_state(&tTX_pre_drive);
        } else {
            state_machine_next_state(&tRX);
        }
    }
}

void tTX_pre_drive_init() {
    GPIO_PinOutSet(PTT_OUT_PORT, PTT_OUT_PIN);
    state_machine_start_timer(100);
    uart_tx("STATE RX (pre_drive)\r\n");
}


void tTX_pre_drive_task() {
    if (state_machine_interrupt_flag) {
        if (GPIO_PinInGet(PTT_PORT,PTT_PIN)) {
            state_machine_next_state(&tTX);
        } else {
            state_machine_next_state(&tTX_ampoff);
        }
    }
}


void tTX_init() {
    vdac_set_gate_bias(ud_get_cal_value());
    uart_tx("STATE TX \r\n");
    GPIO_PinOutSet(LED_PORT, LED_PIN);
}


void tTX_task() {
    if (!GPIO_PinInGet(PTT_PORT,PTT_PIN)) {
        state_machine_next_state(&tTX_ampoff);
    }
}

void tTX_ampoff_init() {
    GPIO_PinOutClear(LED_PORT, LED_PIN);
    GPIO_PinOutClear(PTT_OUT_PORT, PTT_OUT_PIN);
    vdac_set_gate_bias(0);

    state_machine_start_timer(100);
    uart_tx("STATE TX (ampoff)\r\n");
}


void tTX_ampoff_task() {
    if (state_machine_interrupt_flag) {
        if (GPIO_PinInGet(PTT_PORT,PTT_PIN)) {
            state_machine_next_state(&tTX);
        } else {
            state_machine_next_state(&tRX);
        }
    }
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

    GPIO_PinModeSet(PTT_PORT, PTT_PIN, gpioModeInput, 0);

    GPIO_PinModeSet(TX_CONSENT_PORT, TX_CONSENT_PIN, gpioModePushPull, 1);

    GPIO_PinModeSet(PTT_OUT_PORT, PTT_OUT_PIN, gpioModePushPull, 0);

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

    state_machine_init();
    state_machine_next_state(&tTX_ampoff);

    Delay(3000);

    ud_check_version_and_update();

    while (!ud_get_cal_value()) {
        // no cal data, run calibration
        uint8_t err = calibration();
        if (err) {
            // cal failed
            blink_start(5);
            Delay(3000);
            blink_stop();
            Delay(1000);
            m('f');
            m(err);
            Delay(2000);
        }
    }


    while (1) {
        if (e_stop) goto E_STOP;

        state_machine_worker();

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
    blink_start(5);
    while (1);
}
