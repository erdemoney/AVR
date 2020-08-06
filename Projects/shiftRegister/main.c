
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


#define BLINK  (1<<PC1)
#define RCLK   (1<<PC0)
#define SS     (1<<PB2)
#define SER    (1<<PB3)
#define SRCLK  (1<<PB5)


static void update()
{
    PORTC |=  RCLK;
    PORTC &= ~RCLK;
}



#define USE_SPI


#ifdef USE_SPI
    static void display( uInt16 n )
    {
        SPDR = n;
        while (!(SPSR & (1<<SPIF))) {}
        update();
    }
#else
    static void shift_bit(Bool i)
    {
        if (i) { PORTB |=  SER; }
          else { PORTB &= ~SER; }

        PORTB |=  SRCLK;
        PORTB &= ~SRCLK;
    }

    static void display( uInt16 n )
    {
        uInt8 i;

        for (i=0; i<8; i++) {
            shift_bit( n&1 );
            n >>= 1;
        }
        
        update();
    }
#endif





int main()
{        
    stdSetup();

   /*
    * Define serial update clock
    * and test LED as output:
    */
    DDRC |= RCLK | BLINK;
    
#ifdef USE_SPI
   /*
    * Wake up SPI from sleep mode,
    * and define SPI  MOSI and CLK pins as output.
    * (We named them SER and SRCLK here).
    * Note (atmega manual, Section 18.3.2),
    * SS must either be output, or held high.
    * So we set it to output:
    */
    PRR  &= ~(1<<PRSPI);
    DDRB |= (SER | SRCLK | SS);

   /*
    * Enable SPI, Master, set clock rate fck/4,
    * SRCLK lo-hi-lo:
    */
    SPCR = (1<<SPE) | (1<<MSTR) | (1<<CPOL);

#else
   /*
    * Define 'manual' SPI pins as output:
    */
    DDRB |= (SER | SRCLK);
#endif

    uInt16 i=0;

    while (1) {
        PORTC ^= BLINK;
        display( i++ );
        stdThreadSleep(stdSECOND);
    }
        
    return 0;
}
 
