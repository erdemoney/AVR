
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "stdThreads.h" 
#include "stdIR.h" 


/*----------------------------- Threading Setup -----------------------------*/

/*
 * See comments on relative priority between
 * main and receiver threads in main function below:
 */
static void receiver();

stdInstantiateThread( receiverThread, 180, receiver, 0, 0, Null );
stdInstantiateThread( mainThread,       1, Null,    10, 1, Null );

/*
 * Run Queue Initialization:
 */
stdThread_t    stdCurrentThread   = &mainThread;
ThreadPrioQ_t  stdRunQ            = &mainThread;

/*-------------------- Indicator LEDS and Input Buttons ---------------------*/

#define WHITE_N_BRIGHT  (1<<PC5)

#define PROGRAM         (1<<PC4)

/* -------------------------------- Example -------------------------------- */

        SIGNAL(TIMER1_COMPA_vect)
        { IRPulseTimer1COMPA(); }
        
        SIGNAL(TIMER1_CAPT_vect)
        { IRRecTimer1CAPT(); }


#define PERIOD         30    // seconds
#define DELAY           4    // 1/4 seconds
#define BURST           4    // amount
#define NN             16
#define ABORT_VALUE    0x12345678

stdInstantiateSemaphore( lock, 1 );


/* ---------- . ---------- */

#define SEED 0x345

static uInt randomNumber()
{
    
    static uInt32 m_z=  SEED;
    static uInt32 m_w= ~SEED;

    m_z = 36969 * (m_z & 65535) + (m_z >> 16);
    m_w = 18000 * (m_w & 65535) + (m_w >> 16);

    return (m_z << 16) + m_w;  /* 32-bit result */
}

/* ---------- . ---------- */

static uInt32 IRcodes    [NN];
static Bool   longLeaders[NN];
static uInt8  firstCode,lastCode;

static void addIRcode( Bool longLeader, uInt32 IRcode )
{
    IRcodes    [lastCode] = IRcode;
    longLeaders[lastCode] = longLeader;
    
    lastCode = (lastCode+1)%NN;
    if (lastCode == firstCode) { firstCode = (firstCode+1)%NN; }
}

static uInt8 nrofCodes() 
{
    return (lastCode - firstCode + NN) % NN;
}

static void getRandomCode( Bool *longLeader, uInt32 *IRcode )
{
    uInt8 nrcod = nrofCodes();
    
    if (nrcod == 0) {
       *IRcode     = ABORT_VALUE;
       *longLeader = False;
    } else {
        uInt8 random = randomNumber();
        uInt8 offset = random % nrcod;
        uInt8 index  = (firstCode + offset) % NN;
        
       *IRcode     = IRcodes    [ index ];
       *longLeader = longLeaders[ index ];
    }
}

/* ---------- . ---------- */


static void receiver()
{
    while (True) {
        if ( PINC & PROGRAM ) {
            stdThreadSleep(stdSECOND);
        } else {
            stdSemP(&lock);
            {
                Bool   longLeader;
                uInt32 IRcode= IRReceive(&longLeader);

                if (IRcode != ABORT_VALUE) {
                    addIRcode(longLeader,IRcode);

                    PORTC ^= WHITE_N_BRIGHT;
                    stdThreadSleep(stdSECOND/16);
                    PORTC ^= WHITE_N_BRIGHT;
                }
            }
            stdSemV(&lock);
        }
    }
}


int main()
{    
    stdSetup();

    // Enable debug LED pin
    DDRC   =  WHITE_N_BRIGHT;
    PORTC &= ~WHITE_N_BRIGHT;
    
    // Enable pullup for programming pin
    PORTC |= PROGRAM;

    // Start receiver thread
    stdThreadResume(&receiverThread);

    while (True) {
        uInt8 i;
        uInt8 randomSleep = randomNumber()%PERIOD;
        uInt8 randomBurst = randomNumber()%BURST;

        stdThreadSleep( (5 + randomSleep) * stdSECOND );

       /*
        * The following relies on the fact
        * that the receiver thread has a lower
        * prio than this sender thread: when
        * the receiver is aborted this main
        * thread will get the processor until
        * it blocks on the lock.
        */
        IRReceiveAbort(ABORT_VALUE);
        
        stdSemP(&lock);
        {
            for (i=0; i<=randomBurst; i++) {
                uInt8 randomDelay = randomNumber()%DELAY;
                stdThreadSleep( stdSECOND * (1+randomDelay) / 4 );
                
                Bool   longLeader;
                uInt32 IRcode;

                getRandomCode(&longLeader,&IRcode);

                if (IRcode != ABORT_VALUE) {
                    IRSendNECCode(IR_OC1B,DC_33,IRcode,longLeader,True);

                    PORTC ^= WHITE_N_BRIGHT;
                    stdThreadSleep(stdSECOND/16);
                    PORTC ^= WHITE_N_BRIGHT;
                    stdThreadSleep(stdSECOND/16);
                    PORTC ^= WHITE_N_BRIGHT;
                    stdThreadSleep(stdSECOND/16);
                    PORTC ^= WHITE_N_BRIGHT;
                }
            }
        }
        stdSemV(&lock);
    }
    
    return 0;
}
 
