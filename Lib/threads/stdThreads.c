/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *
 *         This module implements a microkernel for use on the AVR microcontroller.
 */

/*--------------------------------- Includes --------------------------------*/

#include "stdThreads.h"
#include "stdDefs.h"

/*------------------------------- Module State ------------------------------*/

/*
 * Number of ticks that task can 
 * run before it will be yielded:
 */
#define TIME_SLICE_QUOTA    4

static ThreadPrioQ_t  timerQ         = Null;
static uInt16         kernelTicks    = 0;
static Bool           sleepPrevent   = False;
       Bool           stdSchedLock   = False;

/*------------------------- Prioritized Task Queues -------------------------*/

static void enQueue( ThreadPrioQ_t *queue, stdThread_t thread )
{
    uInt8 priority= thread->priority;                    

    while ( (*queue) 
         && (*queue)->priority >= priority
      ) { 
        queue= &((*queue)->next); 
    }
    
    thread->next = *queue;                                             
   *queue        = thread;                                                   
}

#define QUEUEHEAD(queue)         queue
#define DEQUEUE(queue)           queue= queue->next;
#define ENQUEUE(queue,thread)    enQueue( &(queue), thread );

/*
 * Function        : Prevent or allow processor going to sleep when idle.
 * Parameters      : preventSleep (I) When True, the processor will *not* go
 *                                    power saving mode when no thread is runnable;
 *                                    such mode is sometimes needed if an AVR
 *                                    device is in use that needs AVR power up 
 *                                    in order to function correctly.
 *                                    When False, the stdThreads scheduler may
 *                                    use its own criteria for deciding whether
 *                                    switching to power save mode is allowed.
 *                                    Note that this scheduler is able to see
 *                                    for most devices whether they are in use
 *                                    or not.
 * Function Result : Old sleep prevention value
 */       
Bool stdSleepPrevent( Bool preventSleep )
{
    Bool result  = sleepPrevent;
    sleepPrevent = preventSleep;
    return result;
}

/*--------------------------- Scheduling Functions --------------------------*/

        #define ALL_DEVICES ( (1<<PRTWI) | (1<<PRTIM2) | (1<<PRTIM1) | (1<<PRTIM0) | (1<<PRSPI) | (1<<PRUSART0) | (1<<PRADC) )

        static void SLEEP()
        {
           /*
            * Infer the sleep mode from the AVR devices 
            * that are currently in use; note that TIMER2 in 
            * asynchronous mode still does run in power save.
            *
            * Any interrupts occurring after the they have
            * been enabled but before the actual sleep instruction 
            * itself will cause SMCR.SE be reset to
            * zero by the interrupt wrapper stdRunISR, 
            * thereby turning the sleep into a noop.
            * That will prevent race conditions by 
            * premature servicing of the interrupts
            * that the sleep instruction is intended 
            * to wait for.
            *
            * NOTE that the following procedure relies on the
            * convention that interrupt service routines never
            * enable devices, or otherwise a deep sleep may
            * be selected where only an idle suddenly becomes
            * required.
            *
            * NOTE: according to Table 9-1 in the ATMega datasheet,
            *       CLKio is disabled in power save mode. Nevertheless,
            *       using Timer2 on the internal 8MHz oscillator 
            *       appears to work fine.
            */
            if ( ( (PRR
                     #if !defined(THREADS_SYSTEM_CLOCK_FREQ_14MHz)
                        |(1<<PRTIM2)
                     #endif
                   ) == ALL_DEVICES )
                   
              &&  (ACSR & (1<<ACD))    // Analog comparator not enabled
              &&  (!sleepPrevent)
               )
               {
                // power save
                SMCR = (1<<SE) | (1<<SM1) | (1<<SM0);
            } else {
                // idle
                SMCR = (1<<SE);
            }

           /*
            * Attempt sleep:
            */
            stdXEnableInterrupts();
            {
                __asm__ __volatile__ ("sleep");
            }
            stdXDisableInterrupts();
        }


/*
 * The following scheduling functions are called 
 * with interrupts disabled. The first one spins 
 * until the run queue becomes non-empty (due to
 * threads made runnable by some interrupt). 
 * While spinning, the interrupts are periodically
 * enabled to allow serving interrupts.
 */
static void deschedule()
{
    stdThread_t self= stdCurrentThread;

    if (!setjmp(*(jmp_buf*)&self->context)) {

        while (!stdRunQ) {
            SLEEP();
        }

        if ( stdRunQ != self ) {
            stdCurrentThread= QUEUEHEAD(stdRunQ);
            longjmp(*(jmp_buf*)&stdCurrentThread->context,1);
        }
    }
}


// Used by interrupt wrapper
void stdReschedule()
{
    if ( !stdSchedLock
      &&  stdRunQ != stdCurrentThread
      && !setjmp(*(jmp_buf*)&stdCurrentThread->context)
       ) {
        stdCurrentThread= QUEUEHEAD(stdRunQ);
        longjmp(*(jmp_buf*)&stdCurrentThread->context,1);
    }
}


/*----------------------------- Thread Functions ----------------------------*/


/*
 * Function        : Suspend execution of the current thread until a corresponding
 *                   stdThreadResume is applied to it.
 *                   NB: thread suspend/resume follows a counting model,
 *                       that is, a thread is runnable iff. the total number of
 *                       resume operations is larger than or equal to the total 
 *                       number of suspend operations (regardless of the order).
 */        
void stdThreadSuspendSelf()
{
    stdXDisableInterrupts();
    {
        if (--stdCurrentThread->runCount == 0) { 
            DEQUEUE(stdRunQ);
            deschedule();   
        }
    }
    stdXEnableInterrupts();
}



/*
 * Function        : Resume suspended thread.
 *                   NB: thread suspend/resume follows a counting model,
 *                       that is, a thread is runnable iff. the total number of
 *                       resume operations is larger than or equal to the total 
 *                       number of suspend operations (regardless of the order).
 * Parameters      : thread  (I) Thread to resume.
 */        
void stdThreadResume( stdThread_t thread )
{
    stdXDisableInterrupts();
    {
        if (++thread->runCount == 1) { 
            ENQUEUE(stdRunQ,thread);
            thread->ticks= TIME_SLICE_QUOTA;
            stdReschedule();   
        }
    }
    stdXEnableInterrupts();
}


/*--------------------------- Semaphore Functions ---------------------------*/

/* 
 * Function        : Acquire semaphore or wait.
 *                   If the semaphore's count is currently equal to zero,
 *                   then wait until this count increases.
 * Parameters      : sem (I) Semaphore to acquire.
 */        
void stdSemP (stdSem_t sem)
{
    Bool canDo;

    stdXDisableInterrupts();
    {
        canDo= sem->count > 0;
    
        if (canDo) {
            sem->count--;
        } else {

            stdThread_t  self= stdCurrentThread;
                    
            DEQUEUE(stdRunQ);
            ENQUEUE(sem->waitQ,self);

            deschedule();   
        }
    }
    stdXEnableInterrupts();
}




/* 
 * Function        : Acquire semaphor or fail.
 *                   Try to acquire a semaphore, and return
 *                   immediate failure if the semaphore's 
 *                   count is currently equal to zero.
 * Parameters      : sem (I) Semaphore to acquire.
 * Function Result : True iff. operation succeeded.
 */        
Bool stdSemTryP (stdSem_t sem)
{
    Bool canDo;

    stdXDisableInterrupts();
    {
        canDo= sem->count > 0;
    
        if (canDo) { 
            sem->count--; 
        }
    }
    stdXEnableInterrupts();
    
    return canDo;
}




/* 
 * Function        : Release semaphore.
 * Parameters      : sem (I) Semaphore to release.
 */        
void stdSemV (stdSem_t sem)
{
    stdXDisableInterrupts();
    {
        stdThread_t revived= QUEUEHEAD(sem->waitQ);
    
        if (sem->count == 0 && revived) {
            DEQUEUE(sem->waitQ);
            ENQUEUE(stdRunQ,revived);
            revived->ticks= TIME_SLICE_QUOTA;
        
            stdReschedule();   
        } else {
            sem->count++;
        }
    }
    stdXEnableInterrupts();
}

/*------------------------------ Time Functions -----------------------------*/

   /*
    * Function        : Timer interrupt handler,
    *                   must be called at stdSECOND Hz.
    */        
    static void stdTimerHandler()
    {
        stdThread_t qHead;

        kernelTicks++;

       /*
        * Process queue of sleeping threads:
        */
        qHead= QUEUEHEAD(timerQ);

        if (qHead) {
            qHead->ticks--;

            while (qHead && !qHead->ticks) {
                DEQUEUE(timerQ);     
                ENQUEUE(stdRunQ, qHead);
                qHead->ticks= TIME_SLICE_QUOTA;
                qHead= QUEUEHEAD(timerQ);
            }
        }

       /*
        * Perform timeslicing if current
        * thread ran out of quota:
        */
        qHead= QUEUEHEAD(stdRunQ);

        if ( qHead && !(--qHead->ticks) ) {
            DEQUEUE(stdRunQ);
            ENQUEUE(stdRunQ, qHead);
            qHead->ticks= TIME_SLICE_QUOTA;
        }
    }



/*
 * Function        : Thread sleep function.
 *                   Delay execution of current thread for at least 
 *                   the specified duration.
 * Parameters      : delay  (I) Minimum amount of time to sleep in ticks of the
 *                              kernel clock. Use stdSECOND for the amount of
 *                              ticks per second.
 */        
void stdThreadSleep (uInt16 delay)
{
    stdThread_t  self = stdCurrentThread;

    stdXDisableInterrupts(); 
    if (delay) {
        ThreadPrioQ_t  *queue = &timerQ;

       /*
        * Insert current thread into 
        * long range time queue:
        */
        while ( (*queue) 
             && (*queue)->ticks <= delay
              ) { 
            delay -= (*queue)->ticks;
            queue= &((*queue)->next); 
        }
    
        if (*queue) {
            (*queue)->ticks -= delay;
        }

        DEQUEUE(stdRunQ);

        self->ticks = delay;
        self->next  = *queue;                                             
       *queue       = self;       

        deschedule();   
    }                  
    stdXEnableInterrupts(); 
}



/*
 * Function        : Current time in kernel clock ticks.
 *                   Use stdSECOND for the amount of ticks per second.
 * Function Result : Current time in kernel clock ticks.
 */        
uInt16 stdTime()
{
    uInt16 result;

   /*
    * Disable interrupts while reading the
    * 16 bit tick counter: the AVR is an 8 bit
    * processor, which cannot read/write 16 (or more) bits
    * from memory in one indivisible operation.
    */
    stdXDisableInterrupts();
    result = kernelTicks;
    stdXEnableInterrupts();

    return result;
}


/*-------------------------- Kernel Initialization --------------------------*/

   /*
    * Setup Timer 2 as System Ticker, 
    *    when cpu and timer driven by a 14745600 Hz external crystal.
    *    Since system clock freq = 14745600 = 2^^16 * 225, using wraparound 
    *    of 225 and a clock prescale of 64 gives a 1024 Hz system timer tick. 
    *
    *    when cpu and timer driven by 8 MHz internal oscillator.
    *    This configuration allows deep powerdown
    *    when no devices (other than this timer)
    *    are used. Even lower power reduction is achieved
    *    by using an external, 32 kHz crystal
    *    (ASYNC mode).
    */
    SIGNAL(TIMER2_COMPA_vect)
    { 
       // See remarks in Section 17.9 of ATMega328 data sheet
       #if defined(THREADS_SYSTEM_TIMER_ASYNC)
           TCCR2A = (1<<WGM21);
       #endif
       
       stdRunISR(stdTimerHandler); 
       
       // See remarks in Section 17.9 of ATMega328 data sheet
       #if defined(THREADS_SYSTEM_TIMER_ASYNC)
           while (ASSR & (1<<TCR2AUB)) {}
       #endif
    }


    #if defined(THREADS_SYSTEM_TIMER_ASYNC) && defined(THREADS_SYSTEM_CLOCK_FREQ_14MHz)
        #error  "Impossible crystal combination"
    #endif


    static void initTimeTicker()
    {
        PRR   &= ~(1<<PRTIM2);

       #if defined(THREADS_SYSTEM_TIMER_ASYNC)
        ASSR   = (1<<AS2);
       #endif

       #if                                                defined(THREADS_SYSTEM_TIMER_FREQ_32Hz)   &&  defined(THREADS_SYSTEM_TIMER_ASYNC)
        TCCR2B = TIMER2_PRESCALE_1024;
        OCR2A  = 0;
       #elif                                              defined(THREADS_SYSTEM_TIMER_FREQ_64Hz)   &&  defined(THREADS_SYSTEM_TIMER_ASYNC)
        TCCR2B = TIMER2_PRESCALE_128;
        OCR2A  = 3;
       #elif                                              defined(THREADS_SYSTEM_TIMER_FREQ_128Hz)  &&  defined(THREADS_SYSTEM_TIMER_ASYNC)
        TCCR2B = TIMER2_PRESCALE_128;
        OCR2A  = 1;
       #elif                                              defined(THREADS_SYSTEM_TIMER_FREQ_256Hz)  &&  defined(THREADS_SYSTEM_TIMER_ASYNC)
        TCCR2B = TIMER2_PRESCALE_128;
        OCR2A  = 0;
       #elif                                              defined(THREADS_SYSTEM_TIMER_FREQ_512Hz)  &&  defined(THREADS_SYSTEM_TIMER_ASYNC)
        TCCR2B = TIMER2_PRESCALE_64;
        OCR2A  = 0;
       #elif                                              defined(THREADS_SYSTEM_TIMER_FREQ_1kHz)   &&  defined(THREADS_SYSTEM_TIMER_ASYNC)
        TCCR2B = TIMER2_PRESCALE_1;
        OCR2A  = 31;
        
       #elif  defined(THREADS_SYSTEM_CLOCK_FREQ_14MHz) && defined(THREADS_SYSTEM_TIMER_FREQ_1kHz)   && !defined(THREADS_SYSTEM_TIMER_ASYNC)
        TCCR2B = TIMER2_PRESCALE_64;
        OCR2A  = 224;
       #elif  defined(THREADS_SYSTEM_CLOCK_FREQ_32kHz) && defined(THREADS_SYSTEM_TIMER_FREQ_1kHz)   && !defined(THREADS_SYSTEM_TIMER_ASYNC)
        TCCR2B = TIMER2_PRESCALE_1;
        OCR2A  = 31;
       #elif  defined(THREADS_SYSTEM_CLOCK_FREQ_32kHz) && defined(THREADS_SYSTEM_TIMER_FREQ_32Hz)   && !defined(THREADS_SYSTEM_TIMER_ASYNC)
        TCCR2B = TIMER2_PRESCALE_1024;
        OCR2A  = 0;
       #elif  defined(THREADS_SYSTEM_CLOCK_FREQ_8MHz)  && defined(THREADS_SYSTEM_TIMER_FREQ_1kHz)   && !defined(THREADS_SYSTEM_TIMER_ASYNC)
        TCCR2B = TIMER2_PRESCALE_1024;
        OCR2A  = 7;
       #elif  defined(THREADS_SYSTEM_CLOCK_FREQ_8MHz)  && defined(THREADS_SYSTEM_TIMER_FREQ_32Hz)   && !defined(THREADS_SYSTEM_TIMER_ASYNC)
        TCCR2B = TIMER2_PRESCALE_1024;
        OCR2A  = 255;
       #else
        #error "unsupported THREADS_SYSTEM_TIMER_FREQ"
       #endif

        TIMSK2 = (1<<OCIE2A);    // Enable Timer2 Compare Match A interrupts
        TCCR2A = (1<<WGM21);     // CTC mode (clear timer on compare match)
        TCNT2  = -1;

       // See remarks in Section 17.9 of ATMega328 data sheet
       #if defined(THREADS_SYSTEM_TIMER_ASYNC)
        while (ASSR & ((1<<TCN2UB)|(1<<OCR2AUB)|(1<<OCR2BUB)|(1<<TCR2AUB)|(1<<TCR2BUB))) {}
       #endif
    }



/*
 * Function        : Setup threading library.
 */        
void stdSetup()
{
   /*
    * Disable CPU clock prescale:
    */
    CLKPR = (1<<CLKPCE);
    CLKPR = 0;
    
    
   /*
    * Initialize all GPIO registers as input, 
    * with pullup resistors enabled.
    * Suggested for power savings in Section 13.2
    * of ATMega328 data sheet:
    */
    DDRB = 0;  PORTB = 0xff;
    DDRC = 0;  PORTC = 0xff;
    DDRD = 0;  PORTD = 0xff;
    
    MCUCR &= ~(1<<PUD);
    
    
   /*
    * Disable Brown Out detection during sleep mode:
    */
    MCUCR |= (1<<BODSE) | (1<<BODS);
    MCUCR |=              (1<<BODS);
    
    
   /*
    * Disable Watchdog:
    */
    MCUSR  &= ~(1<<WDRF);
    WDTCSR |=  (1<<WDCE) | (1<<WDE);
    WDTCSR  =  0x0;
    
    
   /*
    * Shut down all devices,
    * including analog comparator:
    */
    PRR   = ALL_DEVICES;
    ACSR |= (1<<ACD);
    
   /*
    * Set up system timer:
    */
    initTimeTicker();
    
   /*
    * Threads must run with interrupts enabled:
    */
    stdXEnableInterrupts();
}
