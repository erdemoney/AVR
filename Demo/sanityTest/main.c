
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

int main() {

    stdSetup();

    DDRC  = ((1<<PC0)|(1<<PC1)|(1<<PC2)|(1<<PC3)|(1<<PC4)|(1<<PC5));
    DDRB  = ((1<<PB0));

    int i = 0;

    PORTC = 0;

    while (True) {
         for (i=0; i<64; i++ ) {
            stdThreadSleep(stdSECOND/8);
            PORTC = i;
         }
    }

    return 0;
}
