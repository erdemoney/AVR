#ifndef __UART_H
#define __UART_H

#include <inttypes.h>
#include <stdio.h>

void uart_init();
void uart_write(char x);
char uart_read();

void uart_activate( char on );

#endif
