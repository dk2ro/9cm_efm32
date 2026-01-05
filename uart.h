#ifndef EFM32_TEST_UART_H
#define EFM32_TEST_UART_H

#define USER_TX_LOCATION 15 //PC10
#define USER_RX_LOCATION 15 //PC11
#include <stdint.h>


void uart_init(void);
void uart_tx(char *data);
char uart_rx(void);

#endif //EFM32_TEST_UART_H