
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "uart.h"

#include "stdThreads.h"

#define SER PC0
#define SRCLK PC1
#define RCLK PC2

#define WAITTIME (stdSECOND/2)


stdInstantiateThread( mainThread, 1,  Null, 10, 1, NULL );

/*
 * Run Queue Initialization:
 */
stdThread_t    stdCurrentThread   = &mainThread;
ThreadPrioQ_t  stdRunQ            = &mainThread;

void setSerial(uint8_t input) {
    if ( input != 0 ) {
        PORTC |= (1 << SER);
    } else {
        PORTC &= ~(1 << SER);
    }
    stdThreadSleep(WAITTIME);
}

void setSRClock() {
    PORTC |= 1 << SRCLK;
    stdThreadSleep(WAITTIME);
    PORTC &= ~(1 << SRCLK);
    stdThreadSleep(WAITTIME);
}

void setRClock() {
    PORTC |= 1 << RCLK;
    stdThreadSleep(WAITTIME);
    PORTC &= ~(1 << RCLK);
    stdThreadSleep(WAITTIME);
}

void display(uint8_t n, uint16_t numOutputs) {
    for(int i = 0; i < numOutputs; i++) {
        setSerial(n & (1 << i));
        setSRClock();
    }
    setRClock();
}

int main() {

    stdSetup();

    DDRC  = ((1<<PC0)|(1<<PC1)|(1<<PC2)|(1<<PC3)|(1<<PC4)|(1<<PC5));
    DDRB  = ((1<<PB0));

    int i = 0;

    PORTC = 0;
    /*
    while (True) {
        setSerial(i);
        i = !i;
        setClock();
    } 
    */
    display(0b1101, 4);

    return 0;
}
