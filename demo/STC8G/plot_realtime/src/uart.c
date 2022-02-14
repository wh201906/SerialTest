#include "uart.h"

bit UART_busy;
uint8_t UART_buf[UART_BUFLEN];
uint8_t UART_ptr;
char radixTable[] = "0123456789ABCDEF";

void UART_init()
{
  SCON = 0x50;
  T2L = BAUDRATE;
  T2H = BAUDRATE >> 8;
  AUXR = 0x15;
  UART_ptr = 0;
  UART_busy = 0;
  ES = 1;
}

void UART_IRQHandler() interrupt 4
{
  if (TI)
  {
    TI = 0;
    UART_busy = 0;
  }
  if (RI)
  {
    RI = 0;
    UART_buf[UART_ptr++] = SBUF;
    UART_ptr &= 0x0f;
  }
}

void UART_sendByte(uint8_t dat)
{
  while (UART_busy)
    ;
  UART_busy = 1;
  SBUF = dat;
}

void UART_sendStr(uint8_t *str)
{
  while (*str)
    UART_sendByte(*str++);
}

// For more type conversion functions(ftoa(), atoi(), atof()),
// see https://github.com/wh201906/CubeMX_Lib/blob/main/Module/UTIL/util.c
//
// In this function, radix(base) is between 2 and 16
uint8_t myitoa(int32_t val, char *str, uint8_t radix)
{
  // i: the index
  // n: the length(like strlen())

  uint8_t i = 0, n = 0;
  bit isPositive = 1;
  if (radix < 2 || radix > 16)
    return 0;
  if (val < 0)
  {
    val = -val;
    isPositive = 0;
  }
  do
  {
    str[n++] = radixTable[val % radix];
    val /= radix;
  } while (val > 0);
  if (!isPositive)
    str[n++] = '-';
  for (i = 0; i < n / 2; i++) // reverse, use str[n] as tempVar
  {
    str[n] = str[i];
    str[i] = str[n - i - 1];
    str[n - i - 1] = str[n];
  }
  str[n] = '\0';
  return n;
}