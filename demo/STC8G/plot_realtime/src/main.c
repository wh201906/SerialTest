#include <intrins.h>
#include "stc8g.h"
#include "types.h"
#include <math.h>

#include "uart.h"

// configure MAIN_Fosc(in Hz) and Baudrate in uart.h
#define PI 3.1415926535898

int main()
{
  int32_t i;
  char buf[10];

  // init
  UART_init();
  EA = 1; // enable interrupts
  i = 0;

  // loop
  while (1)
  {
    myitoa(i, buf, 10);
    UART_sendStr(buf);
    UART_sendByte(',');
    myitoa(sin((double)i / 256 * 4 * PI) * 10000, buf, 10);
    UART_sendStr(buf);
    UART_sendByte(',');
    myitoa(cos((double)i / 256 * 4 * PI) * 10000 - 2000, buf, 10);
    UART_sendStr(buf);
    UART_sendByte('\n');
    i++;
  }
}