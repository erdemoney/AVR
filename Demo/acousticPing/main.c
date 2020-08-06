
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "uart.h"

#include "stdThreads.h" 

static void ledderF();

stdInstantiateThread( ledder   , 80,  ledderF, 0, 1, NULL );  
stdInstantiateThread( mainThread, 1,  Null,   10, 1, &ledder );

/*
 * Run Queue Initialization:
 */
stdThread_t    stdCurrentThread   = &mainThread;
ThreadPrioQ_t  stdRunQ            = &mainThread;

        static volatile stdThread_t   pingWaiter;
        static volatile uInt32        pingDelay;
        static volatile uInt16        pingPreviousEventTime;

void pingTimer1CAPT()
{ 
    uInt16 icrLo  = ICR1L;
    uInt16 icrHi  = ICR1H;
    uInt16 time   = (icrHi << 8) + icrLo;
    Bool   level  = (ACSR & (1<<ACO))!=0;
    
    if (!level) { 
        pingPreviousEventTime = time;
        TCCR1B |=  (1<<ICES1);                // Trigger on raising edge
    } else {
        TIMSK1  = 0;
        TCCR1B  = 0;
        PRR    |= (1<<PRTIM1);                // disable clock to TIMER1

        ACSR    = (1<<ACD);                   // disable AC
        DIDR1   = 0;                          // Enable digital input on AIN1

        pingDelay = time - pingPreviousEventTime;
        stdThreadResume(pingWaiter); 
    }
}

        SIGNAL(TIMER1_CAPT_vect)
        { pingTimer1CAPT(); }

    static void pingStart()
    {
        // Set Timer 1 in free running mode
        // so that input capture can get timestamps
        // from it. 
        PRR    &= ~(1<<PRTIM1);               // enable clock to TIMER1
        TCCR1B  =  (1<<CS11) | (1<<CS10);     // Prescale 64
        TCCR1B &= ~(1<<ICES1);                // Trigger on falling edge
        TIMSK1  =  (1<<ICIE1);                // Enable input capture

        // Configure analog comparator
        // to compare AIN1 with bandgap voltage:
        //
        ACSR    =         0;                  // enable AC
        ACSR   |=  (1<<ACBG);                 // Use reference voltage Vcc
        ACSR   |=  (1<<ACIC);                 // Enable input capture
        DIDR1  |=  (1<<AIN1D);                // Disable digital input on AIN1
    }

    stdInstantiateSemaphore( lock, 0 );

    static volatile uInt32 currentDelay = 0;
    static volatile uInt32 minDelay     = 999999;
    static volatile uInt32 maxDelay     = 0;
    
    #define RANGE 32
    
    static uInt8 scaledPing()
    {
        stdSemP(&lock);
        uint32_t result = ( (currentDelay - minDelay) * RANGE ) / (maxDelay - minDelay);
        stdSemV(&lock);
    
        return result;
    }
    
static void ledderF()
{
    while (1) {
        if ( (minDelay+10) >= maxDelay) {
            PORTC |=  (1<<PC3);
            stdThreadSleep(RANGE);
        } else {
            uint32_t wait = scaledPing();

            if (wait < 1) {
                PORTC |=  (1<<PC3);
                stdThreadSleep(1);
            } else {
                PORTC &= ~(1<<PC3);
                
                PORTC ^=  (1<<PC4);
                stdThreadSleep( wait );
                PORTC ^=  (1<<PC4);

                stdThreadSleep( RANGE - wait );
            }
        }
    }
}

int main()
{    
    stdSetup();
    
    DDRC  |=  ((1<<PC3) | (1<<PC4) | (1<<PC5));
    PORTC &= ~((1<<PC3) | (1<<PC4) | (1<<PC5));

    uart_init();  

    stdSemV(&lock);

    while (1) {
        stdThreadSleep( stdSECOND/4 );

        pingStart();
        pingWaiter = stdCurrentThread;

        PORTC ^= (1<<PC5);
        stdThreadSleep( 1 );
        PORTC ^= (1<<PC5);
        
        stdThreadSuspendSelf();
        
        stdSemP(&lock);
        if (pingDelay == currentDelay) {
            stdSemV(&lock);
        } else {
            currentDelay = pingDelay;
            if (minDelay > pingDelay) { minDelay = pingDelay; }
            if (maxDelay < pingDelay) { maxDelay = pingDelay; }
            stdSemV(&lock);
            
            printf_P( PSTR(" PING: %d\r\n"),  scaledPing() );
        }
    }
        
    return 0;
}
 
