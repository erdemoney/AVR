/*
 *  Module name              : main.c
 *
 *  Description              :
 *
 */

/*--------------------------------- Includes --------------------------------*/

#include "stdThreads.h" 
#include "stdIR.h" 
#include "stdADC.h" 

/*----------------------------- Threading Setup -----------------------------*/

static void blinkerLoop();
static void beaconLoop();

stdInstantiateThread( beaconThread,  80, beaconLoop,  10, 0,  Null          );
stdInstantiateThread( blinkerThread, 80, blinkerLoop, 10, 1,  Null          );
stdInstantiateThread( mainThread,     1, Null,         0, 1, &blinkerThread );

/*
 * Run Queue Initialization:
 */
stdThread_t    stdCurrentThread   = &mainThread;
ThreadPrioQ_t  stdRunQ            = &mainThread;

/*-------------------- Indicator LEDS and Input Buttons ---------------------*/

#define WHITE_N_BRIGHT  (1<<PC5)
#define ATTENTION       (1<<PC4)
#define BLINK           (1<<PC3)

#define MODE_PIN        PC0

/*---------------------------- Showing Liveness -----------------------------*/

static volatile uInt16 blinkerPeriod = stdSECOND*5;

static void blinkerLoop()
{
    while (True) {
        stdThreadSleep(blinkerPeriod);
        PORTC |=  BLINK;
        stdThreadSleep(stdSECOND/16);
        PORTC &= ~BLINK;
    }
}

/* ---------------------------- Operation Mode ----------------------------- */



                                               // Assume mode pin pulled up with 100k
                                               // NO internal pullup
    typedef enum {                             // ===================================
       modeIR_COMMAND_UNUSED0           =  0,  //      0
       modeIR_COMMAND_UNUSED1           =  1,  //  11111      10k
       modeIR_COMMAND_UNUSED2           =  2,  //  25000      22k
       modeIR_CAMERA_RECEIVER           =  3,  //  42857      47k
       modeIR_REMOTE_THRESHOLD          =  4,  //  66666      63k
       modeIR_BEACON_REMOTE_THRESHOLD   =  5,  // 100000     100k
       modeIR_BEACON                    =  6,  // 150000     150k
       modeIR_COMMAND_UNUSED7           =  7,  // 233333     220k
       modeIR_COMMAND_UNUSED8           =  8,  // 400000     470k
       modeIR_SLOTH_DETECTOR            =  9,  // 900000       1M
       modeIR_INTEGRATED_THRESHOLD      = 10,  // default use, no mode selection registor
       modeNONE                         = 11,
    } OperationMode;

    static OperationMode readRawModePin()
    {
        adcSetup(MODE_PIN);
        OperationMode result= (OperationMode) (( (adcRead()+adcMAX/20) * 10 ) / adcMAX);
        adcTerm();

        DDRD  = (1<<PD0) | (1<<PD1) | (1<<PD2) | (1<<PD3);
        PORTD = result;

        return result;
    }

static OperationMode  readMode()
{
    OperationMode result;

   /*
    * Remove the internal pullup resistor
    * from the mode pin so as to not interfere with the
    * external 100k resistor, and read the pin voltage:
    */
    PORTC &= ~(1<<MODE_PIN);
    result= readRawModePin();
    
    if ( ( result >= modeIR_COMMAND_UNUSED0 )
      && ( result <= modeIR_COMMAND_UNUSED2 )
       ) {
        return modeNONE;
    } else
    if ( ( result >= modeIR_COMMAND_UNUSED7 )
      && ( result <= modeIR_COMMAND_UNUSED8 )
       ) {
        return modeNONE;
    } else {
        return result;
    }
}

/* --------------- Analog Comparator from TSOM 38238 on AIN1 --------------- */

        static Bool    acTriggered;
        static uInt16  acTriggeredTime;
        void acOn()
        {
            // Configure analog comparator
            // to compare AIN1 with bandgap voltage:
            //
            acTriggered = False;
            
            DDRD   &= ~((1<<PD6)|(1<<PD7));        // Define AIN0 as input
            PORTD  &= ~((1<<PD6)|(1<<PD7));        // .. and disable pullup
            ACSR    =   (1<<ACI);                  // enable AC, clear interrupt flag
            ACSR    =   (1<<ACIE);                 // enable AC interrupt
            ACSR   |=   (1<<ACBG);                 // Use 1.1V reference voltage
            ACSR   |=   (1<<ACIS1)|(1<<ACIS0);     // Trigger on falling edge
            DIDR1  |=   (1<<AIN0D)|(1<<AIN1D);     // Disable digital input on AIN01
        }

        SIGNAL(ANALOG_COMP_vect)
        {
            ACSR = (1<<ACD);                      // disable AC
            
            acTriggered     = True;
            acTriggeredTime = stdTime();
        }

/* -------------------- Send fixed amount of IR pulses --------------------- */

       /*
        * IR receiver interrupt:.
        */
        
        SIGNAL(TIMER1_CAPT_vect)
        { IRRecTimer1CAPT(); }


       /*
        * Switch IR beam on with specified duty cycle,
        * wait the specified amount of 38kHz pulses,
        * and then switch off beam again.
        */
            typedef struct {
                uint16_t     strobeDecrement;
                stdThread_t  strobeThread;
                Bool         isBeamStrobe;
            } StrobeInfoRec;
            
            static StrobeInfoRec  strobeInfo[2];
            
                static void wakeStrober0()
                { stdThreadResume(strobeInfo[0].strobeThread); }

                static void wakeStrober1()
                { stdThreadResume(strobeInfo[1].strobeThread); }


            SIGNAL(TIMER0_COMPA_vect)
            { 
                if (strobeInfo[0].isBeamStrobe) {
                    if (strobeInfo[0].strobeDecrement-- == 0) {
                        TIMSK0 &= ~(1<<OCIE0A);
                        stdRunISR(wakeStrober0);
                    }
                } else {
                    IRPulseTimer0COMPA();
                }
            }
        
            SIGNAL(TIMER1_COMPA_vect)
            { 
                if (strobeInfo[1].isBeamStrobe) {
                    if (strobeInfo[1].strobeDecrement-- == 0) {
                        TIMSK1 &= ~(1<<OCIE1A);
                        stdRunISR(wakeStrober1);
                    }
                } else {
                    IRPulseTimer1COMPA();
                }
            }
        
            SIGNAL(TIMER1_COMPB_vect)
            { 
                if (strobeInfo[1].isBeamStrobe) {
                    if (strobeInfo[1].strobeDecrement-- == 0) {
                        TIMSK1 &= ~(1<<OCIE1B);
                        stdRunISR(wakeStrober1);
                    }
                } else {
                    IRPulseTimer1COMPA();
                }
            }
        
        static void sendBeamStrobe( IRPulseOutput pin, DutyCycle duty, uInt16 pulseCount )
        {
            uInt8 pindex = (pin&2)>>1;

            strobeInfo[pindex].isBeamStrobe    = True;
            strobeInfo[pindex].strobeDecrement = pulseCount;
            strobeInfo[pindex].strobeThread    = stdCurrentThread;

            IRPulseStart(pin,duty,True,True);
            stdThreadSuspendSelf();
            IRPulseStop(pin);
        }

        static void sendNECCode( IRPulseOutput pin, uInt32 value, Bool passiveHigh )
        {
            uInt8 pindex = (pin&2)>>1;

            strobeInfo[pindex].isBeamStrobe = False;
            IRSendNECCode(pin,DC_33,value,True,passiveHigh);
        }

/* -------------------- Generic Camera Triggering Logic -------------------- */

        #define TRIGGER_IR_COMMAND             0xee55a

        static void triggerPulse()
        {
           /*
            * Give both modes of camera triggering here,
            * so who listens is depending on the actual wiring:
            */
        
           /*
            * Toggle pin WHITE_N_BRIGHT
            */
            PORTC |=  WHITE_N_BRIGHT;
            stdThreadSleep(stdSECOND/16);
            PORTC &= ~WHITE_N_BRIGHT;
            
           /*
            * Send remote trigger command.
            * If the remote Tx LED is connected
            * that will generate a false detection
            * of the beam strobe. Note that we only
            * trigger Pulses when the beam is interrupted,
            * which then very likely is still the case
            * after Pulse triggering, so simply restart the
            * ac here after a stabilization period:
            */
            sendNECCode(IR_OC1A, TRIGGER_IR_COMMAND,True);
            stdThreadSleep( 1 );
            acOn();
        }
        

        #define PULSE_FREQ                      16
        #define INTERRUPT_COUNT                  2                              // See decideShortBeamInterrupt, lasts two pulses
        #define BEAM_CHECK_FREQ                (PULSE_FREQ/INTERRUPT_COUNT)
        #define UNBLOCKED_RETRIGGER_TICKS      ( 5      * BEAM_CHECK_FREQ)      //  5 sec
        #define BLOCKED_TRIGGER_TICKS          ( 5 * 60 * BEAM_CHECK_FREQ)      //  5 mins

        static void processHALT( Bool beamInterrupt )
        {
            static uInt32 blockedTime = 0;
            
            if (beamInterrupt) {
                if (!(PORTC & ATTENTION)) {
                   /*
                    * Initial blocked detection:
                    */
                    PORTC  |= ATTENTION;

                    blockedTime = 0;
                    triggerPulse();
                } else {
                   /*
                    * Still blocked:
                    */
                    blockedTime++;
                    
                    if ( (blockedTime % BEAM_CHECK_FREQ) == 0 ) {
                        if ( (blockedTime % BLOCKED_TRIGGER_TICKS) == 0 ) {
                           /*
                            * Take another picture every N mins
                            * when blocked:
                            */
                            triggerPulse();
                        }
                    }
                }
            } else {
                if (PORTC &   ATTENTION) {
                    PORTC &= ~ATTENTION;
                    
                    if ( blockedTime >= UNBLOCKED_RETRIGGER_TICKS ) {
                       /*
                        * Take another picture if the blocked
                        * period has been longer than 5 sec:
                        */
                        triggerPulse();
                    }
                }
            }
        }

/* ----------------------- Main IR Thresholds Program ---------------------- */

    #define PULSE_PERIOD                   (stdSECOND/PULSE_FREQ)
    #define PULSE_DELTA                    (1)

   /*
    * Send 38 kHz IR strobe,
    * and measure if it is detected
    * by the sensor:
    */
    static Bool detectBeamInterrupt()
    {
        Bool result;

        acOn();
        sendBeamStrobe(IR_OC1B,DC_12, 8);
        result = !acTriggered;

        return result;
    }

    static void sleepToNextPulse()
    {
        static uInt16 lastPulseTime = 0;

        Int16 delay = lastPulseTime + PULSE_PERIOD - stdTime();

        while (delay < 0) {
            delay += PULSE_PERIOD;
        }

        stdThreadSleep( delay );

        lastPulseTime = stdTime();
    }

    static Bool decideShortBeamInterrupt()
    {
        sleepToNextPulse();

        if (!detectBeamInterrupt()) { 
            return False; 
        }

        sleepToNextPulse();

        return detectBeamInterrupt();
    }

    static Bool decideLongBeamInterrupt()
    {
        static Bool   lockedIn          = True;
        static uInt16 lastTriggeredTime = 0;

        Int16 delay = lastTriggeredTime + PULSE_PERIOD - PULSE_DELTA - stdTime();

        while (delay < 0) {
            delay += PULSE_PERIOD;
        }

        stdThreadSleep( delay );

        if (lockedIn) {
            acOn();
        }

        stdThreadSleep( 2 * PULSE_DELTA );

        lockedIn = acTriggered;

        if (lockedIn) {
            lastTriggeredTime = acTriggeredTime;
        } else {
            lastTriggeredTime = stdTime();
        }

        return !lockedIn;
    }
        
            /* ---- . ---- */
        
    #define VERSION_CODE  0x072818
    static void integratedThresholdLoop()
    {
       /*
        * Send version code via IR:
        */
        sendNECCode(IR_OC1B,VERSION_CODE,True);

        while (True) {
            processHALT( decideShortBeamInterrupt() );
        }
    }

    static void remoteThresholdLoop()
    {
        while (True) {
            processHALT( decideLongBeamInterrupt() );
        }
    }

    static void cameraReceiverLoop()
    {
        while (True) {
            Bool   longLeader;
            uInt32 IRcode= IRReceive(&longLeader);
            if (IRcode == TRIGGER_IR_COMMAND) { triggerPulse(False); }
        }
    }

    static void beaconLoop()
    {
        while (True) {
            stdThreadSleep(PULSE_PERIOD);
            sendBeamStrobe(IR_OC0B,DC_12, 8);
        }
    }

/* ---------------------- Main Sloth Detector Program ---------------------- */

        #define SLOTH_P                   PC1
        #define SLOTH_PIN                (1<<SLOTH_P)
        #define SLOTH_SAMPLE_PERIOD      (stdSECOND / 2)
 
        #define SLOTH_ALPHA                   70  // % of max
        #define SLOTH_BETA                    40  // % of max
        #define SLOTH_SMOOTHING_INTERVAL    3600  // in SAMPLE_PERIODS

        static Bool decideSlothInterrupt()
        {
           /*
            * Wait until next sample time
            */
            stdThreadSleep( SLOTH_SAMPLE_PERIOD );

           /*
            * Sample the pressure sensor; value should
            * drop when pressure is applied:
            */
            adcSetup(SLOTH_P);
            uInt16  level = adcRead();
            adcTerm();
            
           /*
            * Maintain maximum ('no pressure') value as moving average 
            * of all values larger than ALPHA percent of the current max.
            * This allows for gradual changes of the value that we considered
            * to indicate an unloaded sensor. Any value smaller than BETA percent 
            * of the (current) max is considered to indicate a positive pressure;
            * values in between indicate uninteresting pressures (maybe a bird landing?).
            * Take as initial max value the initial sensor reading.
            */
            static Float max;
            static Bool  first=True;
            
            if (first) {
                first = False;
                max   = level;
                return False;
                
            } else {
                if ( level > SLOTH_ALPHA*max ) {
                    max= ( max*(SLOTH_SMOOTHING_INTERVAL-1) + level ) / SLOTH_SMOOTHING_INTERVAL;
                    return False;
                } else {
                    return ( level < SLOTH_BETA*max );
                } 
            }
        }

static void slothDetectorLoop()
{
    DDRC   &= ~SLOTH_PIN;        // Define SLOTH_PIN as input
    PORTC  &= ~SLOTH_PIN;        // .. and disable pullup

    while (True) {
        processHALT( decideSlothInterrupt() );
    }
}

/* --------------------- Main Program, Dispatch Modes ---------------------- */

int main()
{    
    stdSetup();

   /*
    * PCn: All indicator LEDs:
    */
    DDRC   =  (WHITE_N_BRIGHT | ATTENTION | BLINK);
    PORTC &= ~(WHITE_N_BRIGHT | ATTENTION | BLINK);
    
  
   /*
    * Select operation mode:
    */
    switch ( readMode() ) {

       /*******************************************************************************
        * Short IR beam system, with IR transmitter and receiver
        * driven by the same board, as in Mike Hobbs's HALT salamander traps
        */
        case modeIR_INTEGRATED_THRESHOLD:
        {
            integratedThresholdLoop();
        }
        /*******************************************************************************/

       /*******************************************************************************
        * Threshold and camera separated by IR link
        */
        case modeIR_CAMERA_RECEIVER:
        {
            cameraReceiverLoop();
        }
        /*******************************************************************************/

       /*******************************************************************************
        * Long IR beam system, consisting of beacon board and detector board.
        */
        case modeIR_BEACON:
        {    
            beaconLoop();
        }

        case modeIR_REMOTE_THRESHOLD:
        { 
            remoteThresholdLoop();
        }    

        case modeIR_BEACON_REMOTE_THRESHOLD:
        {
            stdThreadResume( &beaconThread );
            remoteThresholdLoop();
        }

        /*******************************************************************************
        * Sloth detector, with pressure sensitive voltage dividor input on pin 13 (SLOTH_PIN)
        */
        
        case modeIR_SLOTH_DETECTOR:
        {
            slothDetectorLoop();
        }

        /*******************************************************************************/
        default:
        {
            blinkerPeriod = stdSECOND/4;
            stdThreadSuspendSelf();
        }
    }
}
 
