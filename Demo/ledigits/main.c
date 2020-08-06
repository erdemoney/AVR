
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "uart.h"

#include "stdThreads.h" 


stdInstantiateThread( mainThread,  1,  Null,        10, 1, Null );

/*
 * Run Queue Initialization:
 */
stdThread_t    stdCurrentThread   = &mainThread;
ThreadPrioQ_t  stdRunQ            = &mainThread;

#define  LOW_SELECT      (1<<PC4)
#define  HIGH_SELECT     (1<<PC5)

#define  SELECT_BITS     (LOW_SELECT|HIGH_SELECT)
#define  DIGIT_BITS      ((1<<PB0)|(1<<PB1)|(1<<PB2)|(1<<PB3)|(1<<PB4)|(1<<PB5)|(1<<PB6))


    static uInt8  lowDisplay;
    static uInt8  highDisplay;

static void ledDisplayF()
{
    while (True) {
        stdThreadSleep( stdSECOND/128 );
        
        PORTC &= ~SELECT_BITS;
        PORTC |= LOW_SELECT;
        PORTB  = ~lowDisplay;
        
        stdThreadSleep( stdSECOND/128 );
        
        PORTC &= ~SELECT_BITS;
        PORTC |= HIGH_SELECT;
        PORTB  = ~highDisplay;
    }
}




uInt8 digits[]= { 95, 12, 59, 62, 108, 118, 119, 28, 127, 126 };

static void printNumber( uInt16 i )
{
    i = i % 100;

    uInt8  low  =  i     % 10;
    uInt8  high = (i/10) % 10;
    
    lowDisplay  = digits[ low  ];
    
    if (!high) { 
        highDisplay = 0;
    } else {
        highDisplay = digits[ high ];
    }
}


    static uInt8 counter;

    SIGNAL(INT0_vect)
    { 
        printNumber( ++counter );
    }
        

int main()
{    
    stdSetup();

    DDRC = SELECT_BITS;
    DDRB = DIGIT_BITS;

    // Inable INT0 interrupt on rising edge
    DDRD  &= ~(1<<PD2);
    EICRA |=  (1<<ISC01) | (1<<ISC00);
    EIMSK |=  (1<<INT0);

    ledDisplayF();
    
    return 0;
}
 
