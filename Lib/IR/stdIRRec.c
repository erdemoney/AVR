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
 *         This module implements IR remote command receiving,
 *         currently NEC protocol only.
 */

/*-------------------------------- Includes ---------------------------------*/

#include <avr/io.h>
#include <avr/pgmspace.h>
#include "stdThreads.h" 
#include "stdIR.h" 

/* ------------------------------ IR Decoding ------------------------------ */

/*
 * NEC protocol
 *
 *
 *                        State transition diagram:
 *                        ========================
 *                                                P1
 *                                                 .--> p0 -+
 *            9ms         4.5ms         P1        /         |
 *      S_0 -------> S_1 -------> S_2 -------> S_E          |
 *                                                \.--> p1 -+
 *       ^                         ^              P3        |
 *       |                         |                        |
 *       \-------------------------+------------------------/
 *
 *      P1 = 1 x 0.5625 ms
 *      P3 = 3 x 0.5625 ms
 */

typedef enum {
   S_0,
   S_1,
   S_2,
   S_E,
   S_p0,
   S_p1,
   S_err,
   S_MAX
} State;

typedef enum {
    P_L,
    P_S,
    P_1,
    P_3,
    P_LLL,
    P_err,
    P_MAX
} Pulse;


State transition[S_MAX][P_MAX] =
{            //   L,     S,     1,     3,     LLL, error
 /* S_0   */  {   S_1,   S_1,   S_err, S_err, S_0, S_err  },
 /* S_1   */  {   S_err, S_2,   S_err, S_err, S_0, S_err  },               
 /* S_2   */  {   S_err, S_err, S_E,   S_err, S_0, S_err  },               
 /* S_E   */  {   S_err, S_err, S_p0,  S_p1,  S_0, S_err  },               
 /* S_p0  */  {   S_err, S_err, S_err, S_err, S_0, S_err  },               
 /* S_p1  */  {   S_err, S_err, S_err, S_err, S_0, S_err  },               
 /* S_err */  {   S_1,   S_err, S_err, S_err, S_0, S_err  },               
};


State   gIRRcvState = S_0;
uInt32  gIRRcvValue;
uInt8   gIRRcvNrBits;
Bool    gLongLeader;        // 9ms: apple remote, 4.5ms: NEC remote


    static Pulse parsePulse( uInt16 pulse )
    {                                                                                             
        if ( (uInt16)(pulse - (P_L_VAL-MAX_ERROR)) < (uInt16)(2*MAX_ERROR) ) { return P_L;   }
        if ( (uInt16)(pulse - (P_S_VAL-MAX_ERROR)) < (uInt16)(2*MAX_ERROR) ) { return P_S;   }                
        if ( (uInt16)(pulse - (P_3_VAL-MAX_ERROR)) < (uInt16)(2*MAX_ERROR) ) { return P_3;   }                
        if ( (uInt16)(pulse - (P_1_VAL-MAX_ERROR)) < (uInt16)(2*MAX_ERROR) ) { return P_1;   }                
        if ( (pulse > P_LLL_VAL) )                                           { return P_LLL; }                                                      
        return P_err;
    }

static void shiftPulse( uInt16 pulseVal, void (*returnReceivedValue)(uInt32,Bool) )
{    
    Pulse   pulse = parsePulse(pulseVal);

    gIRRcvState= transition[gIRRcvState][pulse];

    switch (gIRRcvState) {
    case  S_0     :                                                               break;
    case  S_1     : gIRRcvValue= 0;  gIRRcvNrBits= 0; gLongLeader = (pulse==P_L); break;
    case  S_2     :                                                               break;
    case  S_E     :                                                               break;
    case  S_p0    : gIRRcvValue= (gIRRcvValue>>1);            gIRRcvState= S_2;   break;
    case  S_p1    : gIRRcvValue= (gIRRcvValue>>1)|0x80000000; gIRRcvState= S_2;   break;
    case  S_err   :                                                               break;
    default       :                                                               break;
    }
    
    if (gIRRcvState == S_2 && gIRRcvNrBits++ == 32) {
        returnReceivedValue(gIRRcvValue,gLongLeader);
        gIRRcvState = S_0;
    }
}


/* ------------------------------ IR Receiving ----------------------------- */


        static volatile stdThread_t   IRRecWaiter;
        static volatile uInt32        IRRecReceivedValue;
        static volatile uInt16        IRRecPreviousEventTime;
        static volatile Bool          IRLongLeader;
        
        static void returnReceivedValue( uInt32 value, Bool longLeader )
        {
            stdIFlags intenable;
            
            stdISRLock()
            stdDisableInterrupts(&intenable);
            
            if (IRRecWaiter) {
                IRRecReceivedValue = value;
                IRLongLeader       = longLeader;
                stdThreadResume(IRRecWaiter); 
                IRRecWaiter        = Null;
            }
            
            stdRestoreInterrupts(intenable);
            stdISRUnLock()
        }

 
void IRReceiveAbort(uInt32 abortValue)
{ returnReceivedValue(abortValue,0); }


void IRRecTimer1CAPT()
{ 
    uInt16 icrLo  = ICR1L;
    uInt16 icrHi  = ICR1H;
    uInt16 time   = (icrHi << 8) + icrLo;
    Bool   level  = (ACSR & (1<<ACO))!=0;
               
    uInt16 pulse  = time - IRRecPreviousEventTime;

    IRRecPreviousEventTime = time;
    
    if (level) { TCCR1B &= ~(1<<ICES1); }
         else  { TCCR1B  |= (1<<ICES1); }
         
    shiftPulse(pulse, returnReceivedValue );
}

    static void IRRecStart()
    {
        // Set Timer 1 in free running mode
        // so that input capture can get timestamps
        // from it. On RC5, the amount of Timer 1 
        // cycles per bit is about 12.
        PRR    &= ~(1<<PRTIM1);               // enable clock to TIMER1
        TCCR1B  =  (1<<CS12);                 // Prescale 256
        TCCR1B |=  (1<<ICES1);                // Falling edge
        TIMSK1  =  (1<<ICIE1);                // Enable input capture

        // Configure analog comparator
        // to compare AIN1 with bandgap voltage:
        //
        ACSR    =         0;                  // enable AC
        ACSR   |=  (1<<ACBG);                 // Use reference voltage Vcc
        ACSR   |=  (1<<ACIC);                 // Enable input capture
        DIDR1  |=  (1<<AIN1D);                // Disable digital input on AIN1
    }

    static void IRRecStop()
    {
        TIMSK1  = 0;
        TCCR1B  = 0;
        PRR    |= (1<<PRTIM1);                // disable clock to TIMER1

        ACSR    = (1<<ACD);                   // disable AC
        DIDR1   = 0;                          // Enable digital input on AIN1
    }

uInt32 IRReceive( Bool *longLeader )
{
    IRRecStart();
    
    IRRecWaiter = stdCurrentThread;
    stdThreadSuspendSelf();
    
    IRRecStop();
    
   *longLeader = IRLongLeader;
    return IRRecReceivedValue;
}


