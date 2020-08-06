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

#ifndef stdThreadIncluded
#define stdThreadIncluded

/*-------------------------------- Includes ---------------------------------*/

#include "stdTypes.h"
#include "stdInterrupts.h"
#include "setjmp.h"

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------- Types ----------------------------------*/

/*
 * Provided types:
 */
typedef struct stdThreadRec  *stdThread_t;
typedef struct stdSemRec     *stdSem_t;
typedef struct stdSemRec     *stdMutex_t;
typedef struct stdQueueRec   *stdQueue_t;

typedef stdThread_t           ThreadPrioQ_t;


/*
 * Thread context. 
 * This structure overlays the jump_buf struct
 * (see setjmp.h):
 */
    typedef void (*stdPC)();

typedef struct {
    uInt8       registers[18];
    uInt16      sp;
    uInt8       sr;
    stdPC       pc;
} stdContext_t;


struct stdThreadRec {
    uInt8             priority;
    Int8              runCount;
    stdThread_t       next;
    uInt16            ticks;
    stdContext_t      context;
};

struct stdSemRec {
    Int8              count;
    ThreadPrioQ_t     waitQ;
};

struct stdQueueRec {
    uInt8             first,last;
    uInt8             mask;
    struct stdSemRec  put;
    struct stdSemRec  get;
};


/*-------------------------------- Constants --------------------------------*/

/*
 * Highest priority supported by this kernel.
 */
extern const uInt stdHIGHEST_PRIO;

#define stdHIGHEST_PRIO         250


/*
 * Time constant, amount of ticks of the kernel clock per second.
 */
#if    defined(THREADS_SYSTEM_TIMER_FREQ_1Hz)
    #define stdSECOND     1
#elif  defined(THREADS_SYSTEM_TIMER_FREQ_2Hz)
    #define stdSECOND     2
#elif  defined(THREADS_SYSTEM_TIMER_FREQ_4Hz)
    #define stdSECOND     4
#elif  defined(THREADS_SYSTEM_TIMER_FREQ_8Hz)
    #define stdSECOND     8
#elif  defined(THREADS_SYSTEM_TIMER_FREQ_16Hz)
    #define stdSECOND    16
#elif  defined(THREADS_SYSTEM_TIMER_FREQ_32Hz)
    #define stdSECOND    32
#elif  defined(THREADS_SYSTEM_TIMER_FREQ_64Hz)
    #define stdSECOND    64
#elif  defined(THREADS_SYSTEM_TIMER_FREQ_128Hz)
    #define stdSECOND   128
#elif  defined(THREADS_SYSTEM_TIMER_FREQ_512Hz)
    #define stdSECOND   512
#elif  defined(THREADS_SYSTEM_TIMER_FREQ_1kHz)
    #define stdSECOND  1024
#else
    #error "unsupported THREADS_SYSTEM_TIMER_FREQ"
#endif

/*------------------------------- Module State ------------------------------*/

/*
 * Current thread, and the priority- sorted queue 
 * of runnable threads. These varables are exported
 * so that they can be statically initialized
 * by applications. Under normal circumstances,
 * their values are identical.
 */
extern ThreadPrioQ_t  stdRunQ;
extern stdThread_t    stdCurrentThread;


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
Bool stdSleepPrevent( Bool preventSleep );


/*----------------------------- Thread Functions ----------------------------*/

/*
 * Function        : Macro for statically creating a thread, optionally inserting this into the run queue.
 * Parameters      : name       (I) Name of thread structure variable.
 *                   ssize      (I) Size of call stack in bytes.
 *                   fun        (I) Function to execute (should never terminate).
 *                   prio       (I) Thread priority (higher is more urgent).
 *                   runCount   (I) Runnable counter ( 0 suspends the thread).
 *                   prev       (I) Address of previously created thread structure.
 */       
void  stdInstantiateThread( String name, uInt ssize, Pointer fun, uInt8 prio, uInt8 runCount, stdThread_t prev);

#define stdInstantiateThread(name,ssize,fun,prio,runCount,prev) \
  Byte name##CallStack [ssize ]; \
  struct stdThreadRec name= { prio, runCount, prev, 0, \
                                 { {0},(uInt16)&name##CallStack[(ssize)-1], (1<<SREG_I), (stdPC)fun } \
                                }

/*
 * Function        : Suspend execution of the current thread until a corresponding
 *                   stdThreadResume is applied to it.
 *                   NB: thread suspend/resume follows a counting model,
 *                       that is, a thread is runnable iff. the total number of
 *                       resume operations is larger than or equal to the total 
 *                       number of suspend operations (regardless of the order).
 */        
void stdThreadSuspendSelf();


/*
 * Function        : Resume suspended thread.
 *                   NB: thread suspend/resume follows a counting model,
 *                       that is, a thread is runnable iff. the total number of
 *                       resume operations is larger than or equal to the total 
 *                       number of suspend operations (regardless of the order).
 * Parameters      : thread  (I) Thread to resume.
 */        
void stdThreadResume( stdThread_t thread );


/*--------------------------- Semaphore Functions ---------------------------*/

/*
 * Function        : Macro for statically creating a semaphore.
 * Parameters      : name   (I) Name of semaphore structure variable.
 *                   count  (I) Initial semaphore count.
 */        
void stdInstantiateSemaphore( String name, uInt8 count );

#define stdInstantiateSemaphore(name,count) \
  struct stdSemRec name= { count, Null }


/* 
 * Function        : Acquire semaphore or wait.
 *                   If the semaphore's count is currently equal to zero,
 *                   then wait until this count increases.
 * Parameters      : sem (I) Semaphore to acquire.
 */        
void stdSemP (stdSem_t sem);


/* 
 * Function        : Acquire semaphor or fail.
 *                   Try to acquire a semaphore, and return
 *                   immediate failure if the semaphore's 
 *                   count is currently equal to zero.
 * Parameters      : sem (I) Semaphore to acquire.
 * Function Result : True iff. operation succeeded.
 */        
Bool stdSemTryP (stdSem_t sem);


/* 
 * Function        : Release semaphore.
 * Parameters      : sem (I) Semaphore to release.
 */        
void stdSemV (stdSem_t sem);


/*--------------------------------- Mutexes ---------------------------------*/

/*
 * Function        : Macro for statically creating a mutex.
 * Parameters      : name   (I) Name of mutex structure variable.
 */        
void stdInstantiateMutex( String name );

#define stdInstantiateMutex(name) stdInstantiateSemaphore(name,1)


/* 
 * Function        : Acquire mutex.
 * Parameters      : mutex   (I) Mutex to acquire.
 */        
void stdMutexEnter( stdMutex_t mutex );

#define stdMutexEnter(mutex) stdSemP(mutex)


/* 
 * Function        : Acquire mutex or fail.
 *                   Try to acquire a mutex, and return
 *                   immediate failure if the mutex
 *                   is already entered.
 * Parameters      : mutex (I) Mutex to acquire.
 * Function Result : True iff. operation succeeded.
 */        
Bool stdMutexTryEnter( stdMutex_t mutex );

#define stdMutexTryEnter(mutex) stdSemTryP(mutex)


/* 
 * Function        : Release mutex.
 * Parameters      : mutex   (I) Mutex to release.
 */        
void stdMutexExit( stdMutex_t mutex );

#define stdMutexExit(mutex) stdSemV(mutex)


/*------------------------------ Bounded Queues -----------------------------*/

/*
 * Function        : Macro for statically creating a bounded capacity queue, 
 *                   initialized to empty.
 * Parameters      : name       (I) Name of queue structure variable.
 *                   capacity   (I) Maximal number of elements that the queue can hold.
 *                                  Note: this must be a power of two.
 */        
void stdInstantiateQueue( String name, uInt8 capacity );

#define stdInstantiateQueue(name,capacity) \
  struct __##name##__ {                     \
        struct stdQueueRec queue;           \
        uInt16 contents[capacity];          \
  };\
 struct __##name##__ __##name##__Struct = { { 0,0, (capacity)-1, { capacity, Null }, { 0, Null } } };\
 stdQueue_t name = &__##name##__Struct.queue
 

/* 
 * Function        : Put element in queue or wait.
 *                   Put element into queue; if the number of 
 *                   elements held by the queue has reached its
 *                   capacity, then wait until a slot becomes 
 *                   available. 
 * Parameters      : queue   (I) Queue to put 'element' into.
 *                   element (I) Element to queue.
 */
void stdQueuePut (stdQueue_t queue, uInt16 element);



/* 
 * Function        : Put element into queue or fail.
 *                   Put element into queue, or return immediate
 *                   failure if the queue's capacity has been reached. 
 * Parameters      : queue   (I) Queue to put 'element' into.
 *                   element (I) Element to queue.
 * Function Result : True iff. operation succeeded.
 */        
Bool stdQueueTryPut (stdQueue_t queue, uInt16 element);



/* 
 * Function        : Read element from queue or wait.
 *                   Read element from queue; if the queue is empty,  
 *                   then wait until an element becomes available.
 * Parameters      : queue   (I) Queue to read from.
 *                   element (O) Pointer to result location.
 */        
void stdQueueGet (stdQueue_t queue, uInt16 *element);


/* 
 * Function        : Read element from queue or fail.
 *                   Read element from queue, or return immediate
 *                   failure if the queue is empty.
 * Parameters      : queue   (I) Queue to read from.
 *                   element (O) Pointer to result location.
 * Function Result : True iff. operation succeeded.
 */        
Bool stdQueueTryGet (stdQueue_t queue, uInt16 *element);


/*------------------------------ Time Functions -----------------------------*/

/*
 * Function        : Thread sleep function.
 *                   Delay execution of current thread for at least 
 *                   the specified duration.
 * Parameters      : delay  (I) Minimum amount of time to sleep in ticks of the
 *                              kernel clock. Use stdSECOND for the amount of
 *                              ticks per second.
 */        
void stdThreadSleep (uInt16 delay);


/*
 * Function        : Current time in kernel clock ticks.
 *                   Use stdSECOND for the amount of ticks per second.
 * Function Result : Current time in kernel clock ticks.
 */        
uInt16 stdTime();


/*-------------------------- Kernel Initialization --------------------------*/

/*
 * Function        : Setup threading library.
 */        
void stdSetup();



#ifdef __cplusplus
}
#endif

#endif
