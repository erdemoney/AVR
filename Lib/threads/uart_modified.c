// uart.c
// for NerdKits with ATmega168, 14.7456 MHz clock
// mrobbins@mit.edu

#include <stdio.h>
#include <stdlib.h>
#include "stdThreads.h" 

#include <avr/io.h>
#include <inttypes.h>
#include "uart.h"

static stdThread_t uartTXWaiter;
static stdThread_t uartRXWaiter;


static void uartTXHandler()
{ 
    UCSR0B &= ~(1<<UDRIE0);
    
    if (uartTXWaiter) {
        stdThreadResume(uartTXWaiter);
        uartTXWaiter = NULL;
    }
}

static void uartRXHandler()
{ 
    UCSR0B &= ~(1<<RXCIE0);
    
    if (uartRXWaiter) {
        stdThreadResume(uartRXWaiter);
        uartRXWaiter = NULL;
    }
}


void uart_write(char x) 
{
  if ( (UCSR0A & (1<<UDRE0)) == 0 ) {
      uartTXWaiter = stdCurrentThread;
      UCSR0B |= (1<<UDRIE0);
      stdThreadSuspendSelf();
  }

  UDR0 = x;
}

char uart_read() 
{
  while ( (UCSR0A & (1<<RXC0)) == 0 ) {
      uartRXWaiter = stdCurrentThread;
  
      UCSR0B |= (1<<RXCIE0);
      stdThreadSuspendSelf();
  }

  return UDR0;
}



void uart_activate( char on )
{
   // TODO:
   // Could not find a reliable way to power down and up the uart.
   // Even without doing anything, unplugging the cable and plugging
   // it back in hangs up character reading.
   // So don't do anything for the moment.
}




SIGNAL(USART_UDRE_vect)
{ stdRunISR(uartTXHandler); }

SIGNAL(USART_RX_vect)
{ stdRunISR(uartRXHandler); }



     static int uart_putchar(char c, FILE *stream) {
       uart_write(c);
       return 0;
     }

     static int uart_getchar(FILE *stream) {
       int x = uart_read();
       return x;
     }

void uart_init() {
  // power up uart:
  PRR &= ~(1<<PRUSART0);
    
  // set baud rate
  UBRR0H = 0;
  UBRR0L = 7;   // for 115200bps with 14.7456MHz clock
  // enable uart RX and TX
  
  UCSR0B = (1<<RXEN0)|(1<<TXEN0);
  // set 8N1 frame format
  UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);

  // set up STDIO handlers so you can use printf, etc
  fdevopen(&uart_putchar, &uart_getchar);
}

