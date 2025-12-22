//EFM32 blink test

#ifndef LED_PIN
#define LED_PIN     8
#endif
#ifndef LED_PORT
#define LED_PORT    gpioPortC
#endif

#define PTT_PIN 5
#define PTT_PORT gpioPortA


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_usart.h"
#include "em_vdac.h"

#include "uart.h"

volatile uint32_t msTicks; /* counts 1ms timeTicks */

void Delay(uint32_t dlyTicks);

/**************************************************************************/ /**
 * @brief SysTick_Handler
 * Interrupt Service Routine for system tick counter
 *****************************************************************************/
void SysTick_Handler(void) {
    msTicks++; /* increment counter necessary in Delay()*/
}

/**************************************************************************/ /**
 * @brief Delays number of msTick Systicks (typically 1 ms)
 * @param dlyTicks Number of ticks to delay
 *****************************************************************************/
void Delay(uint32_t dlyTicks) {
    uint32_t curTicks;

    curTicks = msTicks;
    while ((msTicks - curTicks) < dlyTicks);
}


/**************************************************************************/ /**
 * @brief  Main function
 *****************************************************************************/
int main(void) {
    CHIP_Init();

    CMU_ClockEnable(cmuClock_GPIO, true);
    CMU_ClockEnable(cmuClock_VDAC0, true);
    CMU_ClockEnable(cmuClock_HFPER, true);
    CMU_ClockEnable(cmuClock_USART0, true);

    /* Setup SysTick Timer for 1 msec interrupts  */
    if (SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000)) while (1);

    /* Initialize LED driver */
    GPIO_PinModeSet(LED_PORT, LED_PIN, gpioModePushPull, 0);

    GPIO_PinOutSet(LED_PORT, LED_PIN);

    GPIO_PinModeSet(PTT_PORT, PTT_PIN, gpioModeInput, 0);


    VDAC_Init_TypeDef vdacInit = VDAC_INIT_DEFAULT;
    VDAC_InitChannel_TypeDef vdacChInit = VDAC_INITCHANNEL_DEFAULT;
    // Set prescaler to get 1 MHz VDAC clock frequency.
    vdacInit.prescaler = VDAC_PrescaleCalc(1000000, true, 0);
    vdacInit.reference = vdacRefAvdd;
    VDAC_Init(VDAC0, &vdacInit);
    vdacChInit.enable = true;
    VDAC_InitChannel(VDAC0, &vdacChInit, 0);

    VDAC_ChannelOutputSet(VDAC0, 0, 2000);


    uart_init();

    printf("test");

    /* Infinite blink loop */
    uint16_t i = 0;

    char buff[32];

    while (1) {
        sprintf(buff, "Current i = %d\r\n", i++);
        uart_tx(buff);
        Delay(100);
        GPIO_PortOutSetVal(LED_PORT, GPIO_PinInGet(PTT_PORT,PTT_PIN)<< LED_PIN, 1 << LED_PIN);
    }
}
