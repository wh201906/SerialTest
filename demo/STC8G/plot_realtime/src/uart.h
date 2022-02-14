#ifndef _UART_H_
#define _UART_H_

#include <stc8g.h>
#include "types.h"

// SYSClk: 11.0592MHz
#ifndef MAIN_Fosc
#define MAIN_Fosc 11059200L
#endif
// Baudrate: 115200
#define BAUDRATE (65536 - MAIN_Fosc / 115200 / 4)
#define UART_BUFLEN 32

void UART_init();
void UART_sendByte(uint8_t dat);
void UART_sendStr(uint8_t *str);
uint8_t myitoa(int32_t val, char *str, uint8_t radix);

#endif