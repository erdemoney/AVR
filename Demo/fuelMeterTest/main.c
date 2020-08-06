
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "uart.h"

#include "stdThreads.h" 
#include "stdDefs.h" 

stdInstantiateThread( mainThread, 1,  Null,   10, 1, NULL );

/*
 * Run Queue Initialization:
 */
stdThread_t    stdCurrentThread   = &mainThread;
ThreadPrioQ_t  stdRunQ            = &mainThread;


#define FUEL_EMPTY         900
#define FUEL_FULL          2300
#define FUEL_RANGE         2500
#define FUEL_SCALE_RANGE   (FUEL_FULL-FUEL_EMPTY)

static void fuelSetup()
{
    PRR &= ~(1<<PRTIM1);
    
    TCCR1A  = (1<<WGM11);                // Fast PWM
    TCCR1B  = (1<<WGM13)  | (1<<WGM12);  //      TOP = ICR1
    TCCR1A |= (1<<COM1A1);               // PWM Control OC1A, PB1, pin 15
    TCCR1B |= (1<<CS10);                 // Prescale 1

    ICR1    = FUEL_RANGE;
    OCR1A   = FUEL_EMPTY;
    DDRB   |= (1<<PB1);
}

static void fuelSet( uInt16 i )
{
    static uInt16 last;
           uInt16 t=i;
           
    if (last > i) {
        t = i; 
    } else 
    if (i<25) {
        t = i; 
    } else 
    if (i<50) {
        t = i; 
    } else 
    if (i<60) {
        t = i; 
    } else { 
        t = i; 
    }
    
    t = FUEL_EMPTY
          + ((FUEL_SCALE_RANGE/100)*t);
          
    OCR1A = t;
    last  = i;
}



int main()
{    
    stdSetup();
    fuelSetup();
    
    while (1) {
        for (int i=0; i<=100; i+=10) {
            //fuelSet(0);
            //stdThreadSleep(2000);
            fuelSet(i); 
            if ((i%10) == 0) { stdThreadSleep(1000); }
            stdThreadSleep(20);
        }
        stdThreadSleep(3*stdSECOND);

        for (int i=100; i>=0; i-=10) {
            //fuelSet(100);
            //stdThreadSleep(2000);
            fuelSet(i); 
            if ((i%10) == 0) { stdThreadSleep(1000); }
            stdThreadSleep(20);
        }
        stdThreadSleep(3*stdSECOND);
    }
    return 0;
}
 
