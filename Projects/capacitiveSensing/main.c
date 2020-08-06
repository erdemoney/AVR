
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "stdThreads.h" 


stdInstantiateThread( mainThread, 1,  Null, 10, 1, Null );

/*
 * Run Queue Initialization:
 */
stdThread_t    stdCurrentThread   = &mainThread;
ThreadPrioQ_t  stdRunQ            = &mainThread;






int main()
{        
    stdSetup();

    while (True) {
        stdThreadSleep(stdSECOND);
    }
}
 
