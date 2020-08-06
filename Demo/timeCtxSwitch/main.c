/*
 *  Module name              : main.c
 *
 *  Description              :
 *
 *         This example times the amount of context switches 
 *         that can be performed in 10 seconds. Note that the main
 *         thread is given the highest priority, so that the switching
 *         threads are idle when main is preparing going to sleep, and
 *         waking up to save the timers.
 *
 *         On a 14.7 MHz avr, we measured about 28000 switches per second.
 *         This is a context switch time of about 35 us.
 */

/*--------------------------------- Includes --------------------------------*/

#include "stdThreads.h" 
#include "stdInterrupts.h" 

#include "lcd.h"

/* -------------------------------- Example -------------------------------- */

/*
 * Forward declarations:
 */
void switcher1F(); 
void switcher2F();

/*
 * Switchers, initially idle (runCount == 0, 
 * and not linked into ready queue):
 */
stdInstantiateThread( switcher1,       80, switcher1F,    0, 0, Null );
stdInstantiateThread( switcher2,       80, switcher2F,    0, 0, Null ); 


stdInstantiateThread( mainThread,       1, Null,          1, 1, Null );

/*
 * Run Queue Initialization:
 */
stdThread_t    stdCurrentThread   = &mainThread;
ThreadPrioQ_t  stdRunQ            = &mainThread;


volatile uInt16 valL, valH;
volatile Bool   running = True;

void switcher1F()
{
    while (running) {
        stdThreadResume(&switcher2);
        stdThreadSuspendSelf();
        
        stdXDisableInterrupts();
        
        valL += 2;
        
        if (valL == 0) {
            valH++;
        }
        
        stdXEnableInterrupts();
    }
    
    stdThreadSuspendSelf();
} 

void switcher2F()
{
    while (running) {
        stdThreadResume(&switcher1);
        stdThreadSuspendSelf();
    }
    
    stdThreadSuspendSelf();
}

int main()
{    
    // Initialize the kernel
    stdSetup();

    // fire up the LCD
    lcd_init();
    lcd_home();
    
    stdThreadResume(&switcher1);
    
    stdThreadSleep(10*stdSECOND);
    
    uInt16 valL1= valL; 
    uInt16 valH1= valH; 
    
    lcd_write_int16(valH1);
    lcd_write_string(PSTR("/"));
    lcd_write_int16(valL1);

    running = False;

    stdThreadSuspendSelf();
    
    return 0;
}
