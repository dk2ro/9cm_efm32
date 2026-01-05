#include "uart.h"

#include "em_cmu.h"
#include "em_gpio.h"
#include "em_usart.h"



void uart_init(void) {
    CMU_ClockEnable(cmuClock_USART0, true);

    USART_InitAsync_TypeDef initAsync = USART_INITASYNC_DEFAULT;
    initAsync.baudrate = 115200;
    USART_InitAsync(USART0, &initAsync);


    USART0->ROUTEPEN |= USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_TXPEN;
    USART0->ROUTELOC0 = (USART0->ROUTELOC0 &
                         ~(_USART_ROUTELOC0_TXLOC_MASK
                           | _USART_ROUTELOC0_RXLOC_MASK ))
                        | (USER_TX_LOCATION << _USART_ROUTELOC0_TXLOC_SHIFT)
                        | (USER_RX_LOCATION << _USART_ROUTELOC0_RXLOC_SHIFT);

    GPIO_PinModeSet((GPIO_Port_TypeDef)AF_USART0_TX_PORT(USER_TX_LOCATION), AF_USART0_TX_PIN(USER_TX_LOCATION), gpioModePushPull, 1);
    GPIO_PinModeSet((GPIO_Port_TypeDef)AF_USART0_RX_PORT(USER_RX_LOCATION), AF_USART0_RX_PIN(USER_RX_LOCATION), gpioModeInput, 0);
}

void uart_tx(char *data) {
    do{
        USART_Tx(USART0, *data);
    }while (*++data);
}

char uart_rx() {
    if (USART_StatusGet(USART0) & USART_STATUS_RXDATAV) {
        return USART_RxDataGet(USART0);
    }
    return 0;

}