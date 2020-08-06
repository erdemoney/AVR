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

#include "string.h"
/* -------------------------------- Example -------------------------------- */

/*
 * Forward declarations:
 */
void ledDisplayF(); 


/*
 * Static global thread creation.
 * All threads are initially runnable, and linked into the run queue.
 *
 * 80 bytes stack each seems to be sufficient, except for mainThread, 
 * which is running on the program's initial stack:
 */
                  //  NAME        STACK SIZE   FUNCTION     PRIORITY     RUN COUNT    PREVIOUS
                  //  ==========  ==========   ===========  ========     =========    ===========
stdInstantiateThread( ledDisplay,         80,  ledDisplayF,        0,            1,   Null        );  
stdInstantiateThread( mainThread,          1,  Null,               0,            1,   &ledDisplay   );

/*
 * Run Queue Initialization:
 */
stdThread_t    stdCurrentThread   = &mainThread;
ThreadPrioQ_t  stdRunQ            = &mainThread;



/////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    char    c;
    uInt8   size;
    uInt8   image[5];
} CharacterDef;

const CharacterDef font[]= 
      {                                                     
            { ' ', 3, { 0x00, 0x00, 0x00, 0x00, 0x00 } },      
            { 'A', 3, { 0x1e, 0x05, 0x1e, 0x00, 0x00 } },                         
            { 'B', 3, { 0x1f, 0x15, 0x0a, 0x00, 0x00 } },                         
            { 'C', 3, { 0x0e, 0x11, 0x11, 0x00, 0x00 } },                         
            { 'D', 3, { 0x1f, 0x11, 0x0e, 0x00, 0x00 } },                         
            { 'E', 3, { 0x1f, 0x15, 0x15, 0x00, 0x00 } },                         
            { 'F', 3, { 0x1f, 0x05, 0x05, 0x00, 0x00 } },                         
            { 'G', 5, { 0x0e, 0x11, 0x15, 0x1d, 0x04 } },                         
            { 'H', 3, { 0x1f, 0x04, 0x1f, 0x00, 0x00 } },                         
            { 'I', 3, { 0x11, 0x1f, 0x11, 0x00, 0x00 } },                         
            { 'J', 4, { 0x19, 0x11, 0x1f, 0x01, 0x00 } },                         
            { 'K', 4, { 0x1f, 0x04, 0x0a, 0x11, 0x00 } },                         
            { 'L', 3, { 0x1f, 0x10, 0x10, 0x00, 0x00 } },                         
            { 'M', 5, { 0x1f, 0x02, 0x04, 0x02, 0x1f } },                         
            { 'N', 4, { 0x1f, 0x02, 0x04, 0x1f, 0x00 } },                         
            { 'O', 3, { 0x1f, 0x11, 0x1f, 0x00, 0x00 } },                         
            { 'P', 3, { 0x1f, 0x05, 0x07, 0x00, 0x00 } },                         
            { 'Q', 5, { 0x0e, 0x11, 0x15, 0x09, 0x16 } },                         
            { 'R', 4, { 0x1f, 0x05, 0x0d, 0x12, 0x00 } },                         
            { 'S', 3, { 0x12, 0x15, 0x09, 0x00, 0x00 } },                         
            { 'T', 3, { 0x01, 0x1f, 0x01, 0x00, 0x00 } },                         
            { 'U', 3, { 0x1f, 0x10, 0x1f, 0x00, 0x00 } },                         
            { 'V', 5, { 0x07, 0x08, 0x10, 0x08, 0x07 } },                         
            { 'W', 5, { 0x0f, 0x10, 0x0c, 0x10, 0x0f } },                         
            { 'X', 5, { 0x11, 0x0a, 0x04, 0x0a, 0x11 } },                         
            { 'Y', 3, { 0x07, 0x1c, 0x07, 0x00, 0x00 } },                         
            { 'Z', 3, { 0x19, 0x15, 0x13, 0x00, 0x00 } },                         
            { '0', 3, { 0x1f, 0x11, 0x1f, 0x00, 0x00 } },                         
            { '1', 3, { 0x11, 0x1f, 0x10, 0x00, 0x00 } },                         
            { '2', 3, { 0x1d, 0x15, 0x17, 0x00, 0x00 } },                         
            { '3', 3, { 0x15, 0x15, 0x1f, 0x00, 0x00 } },                         
            { '4', 3, { 0x07, 0x04, 0x1f, 0x00, 0x00 } },                         
            { '5', 3, { 0x17, 0x15, 0x1d, 0x00, 0x00 } },                         
            { '6', 3, { 0x1f, 0x15, 0x1d, 0x00, 0x00 } },                         
            { '7', 3, { 0x01, 0x01, 0x1f, 0x00, 0x00 } },                         
            { '8', 3, { 0x1f, 0x15, 0x1f, 0x00, 0x00 } },                         
            { '9', 3, { 0x07, 0x05, 0x1f, 0x00, 0x00 } },                         
            { '.', 2, { 0x18, 0x18, 0x00, 0x00, 0x00 } },                         
            { ',', 2, { 0x14, 0x0c, 0x00, 0x00, 0x00 } },                         
            { '#', 5, { 0x0a, 0x1f, 0x0a, 0x1f, 0x0a } },                         
            { '$', 5, { 0x02, 0x15, 0x1f, 0x15, 0x08 } },                         
            { '*', 5, { 0x04, 0x0e, 0x04, 0x0e, 0x04 } },                         
            { '(', 3, { 0x0e, 0x11, 0x11, 0x00, 0x00 } },                         
            { ')', 3, { 0x11, 0x11, 0x0e, 0x00, 0x00 } },                         
            { '[', 3, { 0x1f, 0x11, 0x11, 0x00, 0x00 } },                         
            { ']', 3, { 0x11, 0x11, 0x1f, 0x00, 0x00 } },                         
            { '{', 4, { 0x04, 0x0e, 0x11, 0x11, 0x00 } },                         
            { '}', 4, { 0x11, 0x11, 0x0e, 0x04, 0x00 } },                         
            { '<', 3, { 0x04, 0x0a, 0x11, 0x00, 0x00 } },                         
            { '>', 3, { 0x11, 0x0a, 0x04, 0x00, 0x00 } },                         
            { '=', 4, { 0x0a, 0x0a, 0x0a, 0x0a, 0x0a } },                         
            { '+', 3, { 0x04, 0x0e, 0x04, 0x00, 0x00 } },                         
            { '-', 3, { 0x04, 0x04, 0x04, 0x00, 0x00 } },                         
            { '|', 1, { 0x1b, 0x00, 0x00, 0x00, 0x00 } },                         
            { '/', 5, { 0x10, 0x08, 0x04, 0x02, 0x01 } },                         
            { '\\', 5,{ 0x01, 0x02, 0x04, 0x08, 0x10 } },                         
            { '~', 5, { 0x04, 0x02, 0x04, 0x08, 0x04 } },                         
            { '%', 5, { 0x13, 0x0b, 0x04, 0x1a, 0x19 } },                         
            { '!', 2, { 0x17, 0x17, 0x00, 0x00, 0x00 } },                         
            { '^', 5, { 0x04, 0x02, 0x01, 0x02, 0x04 } },                         
            { '_', 4, { 0x10, 0x10, 0x10, 0x10, 0x00 } },                         
            { '`', 3, { 0x01, 0x02, 0x04, 0x00, 0x00 } },                         
            { '"', 3, { 0x03, 0x00, 0x03, 0x00, 0x00 } },                         
            { '\'', 1,{ 0x03, 0x00, 0x00, 0x00, 0x00 } },                         
            { '@', 5, { 0x0e, 0x11, 0x17, 0x17, 0x06 } },                         
            { '?', 4, { 0x02, 0x01, 0x15, 0x02, 0x00 } },                         
            { ':', 1, { 0x0a, 0x00, 0x00, 0x00, 0x00 } },                         
    };                                                                           

#define FONT_SIZE 67

const CharacterDef deFault= 
            { '?', 4, { 0x02, 0x01, 0x15, 0x02, 0x00 } };                        

static const CharacterDef *searchChar( char c )
{
    for (int i=0; i<FONT_SIZE; i++) {
        if (font[i].c == c) {
            return &font[i];
        }
    }

    return &deFault;
}


/////////////////////////////////////////////////////////////////////////////////////


#define LEDCPINS ((1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4))
#define LEDBPINS ((1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5))


volatile int ledList[5];

static void clearList()
{
    for (int i = 0; i < 5; i++)
    {
        ledList[i] = 0;
    }
}

static void display( char c )
{
    CharacterDef const  *image = searchChar(c);
    
    for (int i= 0; i<5; i++) {
        ledList[i]= image->image[i];
    }
}

static void displayString(char* input)
{
    uInt16 nextTime= stdTime();
    clearList();
    
    for (int i = 0; i < strlen(input); i++)
    {
        display(input[i]);
        nextTime += stdSECOND;
        stdThreadSleep(nextTime - stdTime()); 
        
        display(' ');
        nextTime += stdSECOND/10;
        stdThreadSleep(nextTime - stdTime()); 
    }
}

static void displayWalkingString(char* input)
{
    uInt16 nextTime= stdTime();

    for (int i = 0; i < strlen(input); i++)
    {
        CharacterDef const  *image = searchChar(input[i]);

        for (int i = 0; i < image->size; i++)
        {
            ledList[0] = ledList[1];
            ledList[1] = ledList[2];
            ledList[2] = ledList[3];
            ledList[3] = ledList[4];
            ledList[4] = image->image[i];
            
            stdThreadSleep(nextTime - stdTime());
            nextTime += stdSECOND/2;

       }
       
       ledList[0] = ledList[1];
       ledList[1] = ledList[2];
       ledList[2] = ledList[3];
       ledList[3] = ledList[4];
       ledList[4] = 0;
  
       stdThreadSleep(nextTime - stdTime());
       nextTime += stdSECOND/2;


    }
}

void ledDisplayF()
{ 
    while (True) {
        for (int b = 1; b <= 5; b++)
        {
            PORTB = ~(1 << b);
            PORTC = ledList[b - 1];
            
            stdThreadSleep(4);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////

int main()
{    
    // Initialize the kernel
    stdSetup();
    
    // LED as output
    DDRC |= ((1<<PC0)|(1<<PC1)|(1<<PC2)|(1<<PC3)|(1<<PC4));
    DDRB |= ((1<<PB1)|(1<<PB2)|(1<<PB3)|(1<<PB4)|(1<<PB5));

    {
        
        uInt16 nextTime= stdTime();
        
        while (True)
        {
            displayWalkingString("HI DAD ");
        }
    }
    
    return 0;
}
