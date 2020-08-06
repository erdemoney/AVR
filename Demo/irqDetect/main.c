
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "uart.h"

#include "stdThreads.h" 

void tickerF();

stdInstantiateThread( ticker,   180,  tickerF,  0, 1, NULL    );
stdInstantiateThread( mainThread, 1,  Null,    10, 1, &ticker );

/*
 * Run Queue Initialization:
 */
stdThread_t    stdCurrentThread   = &mainThread;
ThreadPrioQ_t  stdRunQ            = &mainThread;

stdInstantiateSemaphore( print, 1 );

uint16_t interval_start;

void tickerF()
{
    while (True) {
        stdThreadSuspendSelf();
        stdSemP(&print);
        printf_P( PSTR("    TOCK at %d\n\r"), interval_start );
        stdSemV(&print);
    }
}



    SIGNAL(INT0_vect)
    { 
        stdThreadResume( &ticker );
    }

int main()
{    
    stdSetup();

    uart_init();  

    // Inable INT0 interrupt on rising edge
    DDRD  &= ~(1<<PD2);
    EICRA |=  (1<<ISC01) | (1<<ISC00);
    EIMSK |=  (1<<INT0);

    DDRC  |=  (1<<PC0);

    uint16_t  nextTick = stdTime();
    uint16_t  counter  = 0;

    while (1) {
        uint16_t i;
    
        interval_start = 0;
    
        for ( i=0; i<300; i++) {
            nextTick += stdSECOND;
            stdThreadSleep( nextTick - stdTime() );
            interval_start++;
            PORTC ^= (1<<PC0);
        }
        
        stdSemP(&print);
        printf_P( PSTR(" ---\n\r") );
        stdSemV(&print);
    }
        
    return 0;
}
 
