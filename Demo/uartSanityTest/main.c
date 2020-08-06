
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "uart.h"

#include "stdThreads.h" 


stdInstantiateThread( mainThread, 1,  Null, 10, 1, NULL );

/*
 * Run Queue Initialization:
 */
stdThread_t    stdCurrentThread   = &mainThread;
ThreadPrioQ_t  stdRunQ            = &mainThread;

#define LED_YELLOW      PC1
#define LED_GREEN       PC2
#define LED_RED         PC3

int main()
{    
    uInt i= 0;
    
    stdSetup();

    DDRC  = ((1<<LED_YELLOW)|(1<<LED_GREEN)|(1<<LED_RED));

    uart_init();  

    while (1) {
        PORTC ^= (1 << LED_YELLOW);
        printf_P( PSTR(" TESTa %d\r\n"), i++ );

        stdThreadSleep(stdSECOND);

        PORTC ^= (1 << LED_GREEN);
        printf_P( PSTR(" TESTb %d\r\n"), i++ );

        stdThreadSleep(stdSECOND);

        PORTC ^= (1 << LED_RED);
        printf_P( PSTR(" TESTc %d\r\n"), i++ );

        stdThreadSleep(stdSECOND);
    }
        
    return 0;
}
 
