
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "uart.h"

#include "stdThreads.h" 

stdInstantiateThread( mainThread, 1,  Null,   10, 1, NULL );

/*
 * Run Queue Initialization:
 */
stdThread_t    stdCurrentThread   = &mainThread;
ThreadPrioQ_t  stdRunQ            = &mainThread;


#define DAC_LADC   (1<<PC2)
#define DAC_SDI    (1<<PC3)
#define DAC_SCK    (1<<PC4)
#define DAC_CS     (1<<PC5)


void dacInit()
{
    DDRC  |=  (DAC_LADC | DAC_SDI | DAC_SCK | DAC_CS);
    
    PORTC &= ~(DAC_SCK);
    PORTC |=  (DAC_LADC | DAC_CS);
}

static void wrdac( uInt8 n )
{
    for (uInt8 i= 0; i<8; i++) {
        if ( ((Int8)n)<0 ) {  PORTC |=  (DAC_SDI); }
                      else {  PORTC &= ~(DAC_SDI); }
        stdThreadSleep(1);
        PORTC |=  (DAC_SCK);
        stdThreadSleep(1);
        PORTC &= ~(DAC_SCK);
        stdThreadSleep(1);
        
        n<<=1;
    }
}

void dacWrite( uInt16 n )
{
    n |= (3<<12);
    PORTC &= ~(DAC_CS);
    stdThreadSleep(1);
    wrdac( n>>8 );
    wrdac( n    );
    PORTC &= ~(DAC_SDI);
    stdThreadSleep(1);
    PORTC |=  (DAC_CS);
    
    PORTC &= ~(DAC_LADC);
    stdThreadSleep(1);
    PORTC |=  (DAC_LADC);
}




int main()
{    
    stdSetup();
    
    dacInit();

    while (1) {
        for (uInt8 i=0; i<40; i+=1) {
            stdThreadSleep( stdSECOND );

            dacWrite(0);
        }
    }
        
    return 0;
}
 
