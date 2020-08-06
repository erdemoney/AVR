
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "stdThreads.h" 
#include "stdIR.h" 


/*----------------------------- Threading Setup -----------------------------*/

stdInstantiateThread( mainThread,      1, Null,    10, 1, Null );

/*
 * Run Queue Initialization:
 */
stdThread_t    stdCurrentThread   = &mainThread;
ThreadPrioQ_t  stdRunQ            = &mainThread;

/*-------------------- Indicator LEDS and Input Buttons ---------------------*/

#define WHITE_N_BRIGHT  (1<<PC5)
#define ATTENTION       (1<<PC4)

#define IS_SENDER_INV   (1<<PC1)

/* -------------------------------- Example -------------------------------- */

        SIGNAL(TIMER1_COMPA_vect)
        { IRPulseTimer1COMPA(); }
        
        SIGNAL(TIMER1_CAPT_vect)
        { IRRecTimer1CAPT(); }


#define MAGIC  0xee55a

int main()
{    
    stdSetup();

    // Set debug LED pins as output
    DDRC   =  ( WHITE_N_BRIGHT | ATTENTION );
    PORTC &= ~( WHITE_N_BRIGHT | ATTENTION );

    // Enable pullup resistors for input pins
    PORTC |=  ( IS_SENDER_INV );


    if ( (PINC & IS_SENDER_INV) == 0 ) {
        while (True) {
            stdThreadSleep(stdSECOND);

            IRSendNECCode(IR_OC1B,DC_33,MAGIC,True,False);

            PORTC ^= WHITE_N_BRIGHT;
            stdThreadSleep(stdSECOND/16);
            PORTC ^= WHITE_N_BRIGHT;
        }
    } else {
        while (True) {
            Bool   lldr;
            uInt32 ircd = IRReceive(&lldr);
            uInt8  led  = (ircd == MAGIC) ? WHITE_N_BRIGHT : ATTENTION;

            PORTC ^= led;
            stdThreadSleep(stdSECOND/16);
            PORTC ^= led;
        }
    }
}
 
