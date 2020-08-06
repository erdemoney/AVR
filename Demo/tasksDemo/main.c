/*
 *  Module name              : main.c
 *
 *  Description              :
 *
 *         This example demonstrates the use of the micro kernel
 *         on the AVR. It performs some simple multithreading: 
 *         three independent ticker tasks (periodically sleeping),
 *         one producer with two consumers, plus a led counter thread
 *         that repeatedly counts from 0 to 15 and displays the result
 *         in binary via 4 output pins.
 * 
 *         The producer produces 'tokens' in the token semaphore,
 *         to be picked up by either consumer. The producer's
 *         production is also bound by a control semaphore, to 
 *         prevent runaway.
 *
 *         All threads print in parallel to their corresponding areas 
 *         on the lcd screen, which needs a lock to avoid unwanted printing
 *         interaction. This lock is created in a locked, or 'held'
 *         state, to be released by the main thread after lcd
 *         initialization is performed.
 */

/*--------------------------------- Includes --------------------------------*/

#include "stdThreads.h" 
#include "stdInterrupts.h" 

#include "lcd.h"

/* ---------------------------------- I/O ---------------------------------- */

/*
 * lcd printing lock. Hold this lock until the lcd is initialized:
 */
stdInstantiateSemaphore( print, 0 );

void PRINTS(uInt8 row, uint8_t col, const char *s)
{
    stdSemP(&print);
    lcd_goto_position(row,col);
    lcd_write_string(s);
    stdSemV(&print);
}

void PRINT(uInt8 row, uint8_t col, const char *s, uInt16 val)
{
    stdSemP(&print);
    lcd_goto_position(row,col);
    lcd_write_string(s);
    lcd_write_string(PSTR(": "));
    lcd_write_int16(val);
    lcd_write_string(PSTR("  "));
    stdSemV(&print);
}

/* -------------------------------- Example -------------------------------- */

/*
 * Forward declarations:
 */
void ledCounterF(); 
void ticker1F(); 
void ticker2F();
void producerF();
void consumer1F(); 
void consumer2F();


/*
 * Static global thread creation.
 * All threads are initially runnable, and linked into the run queue.
 *
 * 80 bytes stack each seems to be sufficient, except for mainThread, 
 * which is running on the program's initial stack:
 */
                  //  NAME        STACK SIZE   FUNCTION     PRIORITY     RUN COUNT    PREVIOUS
                  //  ==========  ==========   ===========  ========     =========    ===========
stdInstantiateThread( ledCounter,         80,  ledCounterF,        0,            1,   Null        );  
stdInstantiateThread( consumer1,          80,  consumer1F,         0,            1,   &ledCounter );  
stdInstantiateThread( consumer2,          80,  consumer2F,         0,            1,   &consumer1  );
stdInstantiateThread( ticker1,            80,  ticker1F,           0,            1,   &consumer2  );
stdInstantiateThread( ticker2,            80,  ticker2F,           0,            1,   &ticker1    ); 
stdInstantiateThread( producer,           80,  producerF,          0,            1,   &ticker2    );
stdInstantiateThread( mainThread,          1,  Null,               0,            1,   &producer   );

/*
 * Run Queue Initialization:
 */
stdThread_t    stdCurrentThread   = &mainThread;
ThreadPrioQ_t  stdRunQ            = &mainThread;


/*
 * Static global lock creation:
 */
stdInstantiateSemaphore( tokens,  0 );
stdInstantiateSemaphore( control, 5 );


/*
 * Thread activation functions. 
 * These must not terminate:
 */
void ledCounterF()
{
    uInt i= 0;
    
    // LED as output
    DDRC |= ((1<<PC0)|(1<<PC1)|(1<<PC2)|(1<<PC3));

    while (True) {
        stdThreadSleep(stdSECOND/2);
        
        i= (i+1) % 16;
        
        PORTC = i;
        PRINT(3,0,PSTR("  LEDS "),i);
    }
}

void producerF()
{
    uInt produced= 0;
    
    while (True) {
        stdSemP(&control);
        produced++;
        PRINT(0,10,PSTR("P "),produced);
        stdSemV(&tokens);
   }
}

void consumer1F()
{
    uInt consumed= 0;
    
    while (True) {
        stdSemP(&tokens);
        consumed++;
        PRINT(1,0,PSTR("C1"),consumed);
        stdSemV(&control);
    }
}

void consumer2F()
{
    uInt consumed= 0;
    
    while (True) {
        stdSemP(&tokens);
        consumed++;
        PRINT(1,10,PSTR("C2"),consumed);
        stdSemV(&control);
    }
}

void ticker1F()
{
    uInt token= 0;
    
    while (True) {
        stdThreadSleep(2*stdSECOND);
        token = !token;
        PRINT(2,0,PSTR("T1"),token);
    }
} 

void ticker2F()
{
    uInt token= 0;
    
    while (True) {
        stdThreadSleep(3*stdSECOND);
        token = !token;
        PRINT(2,10,PSTR("T2"),token);
    }
}

int main()
{    
    // Initialize the kernel
    stdSetup();

    // fire up the LCD
    lcd_init();
    lcd_home();
    
    // release print lock
    stdSemV(&print);
    
    {
       /*
        * The following shows how to implement a 
        * clock function. Merely sleeping for one
        * second in each loop iteration (like the 
        * above ticker tasks do) will cause time drift. 
        * The proper way to do this is to plot out
        * a timeline at which the clock ticks should occur
        * and then repeatedly sleep until the next clock tick:
        */
        uInt16 nextTime= stdTime();

        while (True) {
            nextTime += stdSECOND;
            stdThreadSleep(nextTime - stdTime());
            PRINTS(0,0,PSTR("          "));

            nextTime += stdSECOND;
            stdThreadSleep(nextTime - stdTime());
            PRINTS(0,0,PSTR("HELLO DAAN"));
        }
    }
    
    return 0;
}
