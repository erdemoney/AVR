/*
 *  Module name              : main.c
 *
 *  Description              :
 *
 *         This example demonstrates a driver
 *         for a 2 digit led display. 
 */

/*--------------------------------- Includes --------------------------------*/

#include "stdThreads.h" 
#include "stdInterrupts.h" 
#include "stdADC.h" 

#include "lcd.h"

/* -------------------------- Threading Structure -------------------------- */

/*
 * Forward declarations:
 */
static void displayF();


/*
 * Static global threads creation.
 * All threads are initially runnable, and linked into the run queue.
 * 80 bytes stack each seems to be sufficient, except for mainThread, 
 * which is running on the program's initial stack
 *
 * Note that we have two threads here: the main 'processing' thread
 * and the 2-digit LED display driver thread that is responsible for
 * displaying the numbers set by the main thread.
 * We chose here to give LED display the lowest priority (= higher value),
 * which sacrifices display for processing whenever the processor
 * gets overloaded:
 */
                  //  NAME        STACK SIZE   FUNCTION     PRIORITY     RUN COUNT    PREVIOUS
                  //  ==========  ==========   ===========  ========     =========    ===========
stdInstantiateThread( display,            80,  displayF,          10,            1,   Null      );
stdInstantiateThread( mainThread,          1,  Null,               0,            1,   &display  );

/*
 * Run Queue Initialization:
 */
stdThread_t    stdCurrentThread   = &mainThread;
ThreadPrioQ_t  stdRunQ            = &mainThread;


/* ------------------------------ LED display ------------------------------ */

#define ALL_CTRL        0x30
#define ALL_CLEDS       0x0e
#define ALL_BLEDS       0x1e

static void ledSetup()
{
    // LEDs as output
    DDRC |= ALL_CTRL;
    DDRC |= ALL_CLEDS;
    DDRB |= ALL_BLEDS;
}

/*
 * Digit masks, describing values for
 * port B and C, as in 0xbc, for each 
 * decimal digit. Note that these masks
 * reflect how the different segments are
 * wired up to the processor's IO ports:
 */
//uint8_t  digitMask[] = { 0xed, 0x21, 0x6e, 0x67, 0xa3, 0xc7, 0xcf, 0x61, 0xef, 0xe7 };
  uint8_t  digitMask[] = { 0xfc, 0x90, 0x3e, 0xb6, 0xd2, 0xe6, 0xee, 0xb0, 0xfe, 0xf6 };

/*
 * Set the segments pattern of a single digit, 
 * either the left or the right one.
 *
 * Note that the board connects the pins of the
 * 'same' segments for the left and right digits,
 * and it also selects the common anodes of the
 * left and right digits as a whole by PC4 and PC5 
 * respectively. That allows selecting/deselecting
 * the left and right digits as a whole by manipulating
 * PC4 and PC5.
 *
 * This wiring makes it impossible to show different
 * segment patterns at the same time. So we can either
 * show a left digit while blanking the right one,
 * or a right digit while blanking the left one,
 * or the same digit left and right.
 * This function does not support the latter,
 * i.e. same digit left and right.
 *
 * Also note that this function is a helper function
 * used by the display thread. By rapidly switching
 * between the left and right digit this thread makes
 * it appear as if both digits show a, possibly different,
 * value at the same time.
 */
static void setDigit( Bool left, uint8_t m )
{
    PORTB = (PORTB & ~ALL_BLEDS) | ((m>>3) & ALL_BLEDS);
    PORTC = (PORTC & ~ALL_CLEDS) | ((m>>0) & ALL_CLEDS);
    PORTC = (PORTC & ~ALL_CTRL)  | (left?0x10:0x20);
}

/*
 * The segment mask left and right values 
 * to be displayed by the display thread.
 * These values are 'computed' by the different
 * display functions:  ledSetNumber or ledSetRandomPattern,
 * whichever is currently chosen in the main() loop.
 */
volatile int mask_left, mask_right;


static void displayF()
{
    while (True) {
        setDigit(True,  mask_left);
        stdThreadSleep( stdSECOND / 64 );

        setDigit(False, mask_right);
        stdThreadSleep( stdSECOND / 64 );
    }
}

/* -------------------- Setting Full Number for Display -------------------- */

static void ledSetNumber( uint8_t n )
{
    mask_left  = ~digitMask[ (n/10)%10 ];  // complement of digit mask, because
    mask_right = ~digitMask[ (n/ 1)%10 ];  //     we have a common anode display
}

/* -------------------------- ADC from LM34 on PC0 ------------------------- */

static float readFahrenheit()
{
   /*
    * (5000 mV / 1024 steps) * (1 deg / 10mV):
    */
    return adcRead() * ( 5000.0 /1024.0 / 10.0 );
}

/* ------------------------------ Main Program ----------------------------- */


int main()
{    
    // Initialize the kernel
    stdSetup();
 
    // Initialize the led displayer
    ledSetup();
 
    // Initialize the adc
    adcSetup();
         
   /*
    * Demo program, counting on the
    * LED display:
    */
    while (True) {
        ledSetNumber  ( readFahrenheit() );
        stdThreadSleep(  5 * stdSECOND   );
    }

    return 0;
}
