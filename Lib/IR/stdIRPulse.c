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
 *         This module implements IR remote command sending,
 *         currently NEC protocol only.
 */

/*-------------------------------- Includes ---------------------------------*/

#include <avr/io.h>
#include <avr/pgmspace.h>
#include "stdThreads.h" 
#include "stdIR.h" 

/* -------------- IR Pulse Transmission via COM 09349 on IRTX --------------- */

static void IRLevelSwitch( IRPulseOutput pin )
{
    if ( (pin & 2) == 0 ) {
        if (pin == IR_OC0A__) {
            TCCR0A ^= ((1<<COM0A1)|(1<<COM0A0));
        } else {
            TCCR0A ^= ((1<<COM0B1));
        }

    } else {
        if (pin == IR_OC1A) {
            TCCR1A ^= ((1<<COM1A1)|(1<<COM1A0));
        } else {
            TCCR1A ^= ((1<<COM1B1));
        }
    }
}

void IRPulseStart( IRPulseOutput pin, DutyCycle duty, Bool on, Bool passiveHigh )
{
    if ( (pin & 2) == 0 ) {
        uInt8 IRTX;

        if (pin == IR_OC0A__) {
            IRTX = PD6;
        } else {
            IRTX = PD5;
        }

        PRR    &= ~(1<<PRTIM0);
        DDRD   |=  (1<<IRTX);               // pin as out.
        
        if (passiveHigh) {
            PORTD  |=  (1<<IRTX);           // pin high
        } else {
            PORTD  &= ~(1<<IRTX);           // pin low
        }

        TCNT0   = 0;
        TCCR0B  =  (1<<CS01);               // Prescale 8
        OCR0A   = DC_100;                   // Compare Match value.
        OCR0B   = DC_100-duty;              // Duty cycle. Note that the IR RX pin is assumed high when idle, so invert the output.
        TCCR0A |= (1<<WGM00) | (1<<WGM01);  // FAST PWM
        TCCR0B |= (1<<WGM02);               // ........
        TIMSK0  = (1<<OCIE0A);              // Enable Timer0 Compare Match A interrupts
    } else {
        uInt8 IRTX;

        if (pin == IR_OC1A) {
            IRTX = PB1;
        } else {
            IRTX = PB2;
        }

        PRR    &= ~(1<<PRTIM1);
        DDRB   |=  (1<<IRTX);               // pin as out.

        if (passiveHigh) {
            PORTB  |=  (1<<IRTX);           // pin high
        } else {
            PORTB  &= ~(1<<IRTX);           // pin low
        }

        TCNT1   = 0;
        TCCR1B  =  (1<<CS11);               // Prescale 8
        ICR1    = DC_100;                   // Compare Match value.
        
        if (pin == IR_OC1A) {
          OCR1A = DC_100-duty;              // Duty cycle. Note that the IR RX pin is assumed high when idle, so invert the output.
          OCR1B = ICR1-1;                   // Interrupt value.
        } else {
          OCR1B = DC_100-duty;              // Duty cycle. Note that the IR RX pin is assumed high when idle, so invert the output.
          OCR1A = ICR1-1;                   // Interrupt value.
        }
        
        TCCR1A |= (1<<WGM11);               // FAST PWM
        TCCR1B |= (1<<WGM13) | (1<<WGM12);  // ........
        
        if (pin == IR_OC1A) {
          TIMSK1= (1<<OCIE1B);              // Enable Timer1 Compare Match B interrupts
        } else {
          TIMSK1= (1<<OCIE1A);              // Enable Timer1 Compare Match A interrupts
        }
    }
    
    if (on) { IRLevelSwitch(pin); }
}

void IRPulseStop( IRPulseOutput pin )
{
    if ( (pin & 2) == 0 ) {
        TCCR0A = 0;
        TCCR0B = 0;
        TIMSK0 = 0;
        TIFR0  = 0;

        if (pin == IR_OC0A__) {
            DDRD  &= ~(1<<PD6);            // IRTX as output off.
        } else {
            DDRD  &= ~(1<<PD5);            // IRTX as output off.
        }

        PRR |= (1<<PRTIM0);
    } else {
        TCCR1A = 0;
        TCCR1B = 0;
        TIMSK1 = 0;
        TIFR1  = 0;

        if (pin == IR_OC1A) {
            DDRB  &= ~(1<<PB1);            // IRTX as output off.
        } else {
            DDRB  &= ~(1<<PB2);            // IRTX as output off.
        }

        PRR |= (1<<PRTIM1);
    }
}

/* --------------------- Send IR code in NEC protocol ---------------------- */

       /*
        * Interrupt driven state machine for strobing 
        * out a 32 bit NEC code:
        */
            typedef struct {
                IRPulseOutput pin;
                uInt32        value;
                stdThread_t   thread;
                uInt16        decrement;
                 Int8         state;
                Bool          atn;
            } IRStrobeInfoRec;
            
            
            static volatile IRStrobeInfoRec  IRStrobeInfo[2];
            
                     
                static void wakeStrober0()
                { 
                    stdThreadResume(IRStrobeInfo[0].thread); 
                    IRStrobeInfo[0].thread = Null;
                }

                static void wakeStrober1()
                { 
                    stdThreadResume(IRStrobeInfo[1].thread); 
                    IRStrobeInfo[1].thread = Null;
                }

          /*
           * Scale the receive pulse values in
           * order to account for the different
           * time unit used for sending:
           */
           #if   defined(THREADS_SYSTEM_CLOCK_FREQ_8MHz)
               #define S(p_val) (uInt16)(1.15*(uInt16)(p_val))
           #elif defined(THREADS_SYSTEM_CLOCK_FREQ_14MHz)
               #define S(p_val) (uInt16)(0.64*(uInt16)(p_val))
           #else
               #error "unsupported THREADS_SYSTEM_CLOCK_FREQ"
           #endif

static void pulseTimerCOMPA( uInt8 timer )
{
    if (IRStrobeInfo[timer].thread) {
        if (IRStrobeInfo[timer].state == -3) {
            IRLevelSwitch(IRStrobeInfo[timer].pin);
            IRStrobeInfo[timer].state++;
        } else
        if (IRStrobeInfo[timer].decrement-- == 0) {
            IRLevelSwitch(IRStrobeInfo[timer].pin);
        
            if (IRStrobeInfo[timer].atn) {
                IRStrobeInfo[timer].decrement = S(P_1_VAL);
                IRStrobeInfo[timer].atn       = False;
            } else
            if (IRStrobeInfo[timer].state == 32) {
                if (timer == 0) {
                    TIMSK0 &= ~(1<<OCIE0A);
                    stdRunISR(wakeStrober0);
                } else {
                    TIMSK1 &= ~(1<<OCIE0A);
                    stdRunISR(wakeStrober1);
                }
            } else
            if (IRStrobeInfo[timer].state == -2) {
                IRStrobeInfo[timer].state++;
                IRStrobeInfo[timer].decrement = S(P_S_VAL);
            } else
            if (IRStrobeInfo[timer].state == -1) {
                IRStrobeInfo[timer].state++;
                IRStrobeInfo[timer].decrement = S(P_1_VAL);
            } else {
                if ( IRStrobeInfo[timer].value & ((uInt32)1 << IRStrobeInfo[timer].state) ) {
                    IRStrobeInfo[timer].decrement = S(P_3_VAL);
                } else {
                    IRStrobeInfo[timer].decrement = S(P_1_VAL);
                }

                IRStrobeInfo[timer].state++;
                IRStrobeInfo[timer].atn = True;
            }
        }
    }
}


void IRPulseTimer0COMPA() { pulseTimerCOMPA(0); }
void IRPulseTimer1COMPA() { pulseTimerCOMPA(1); }


void IRSendNECCode( IRPulseOutput pin, DutyCycle duty, uInt32 value, Bool longLeader, Bool passiveHigh )
{
    uInt8 pindex = (pin&2)>>1;

    IRStrobeInfo[pindex].pin       = pin;
    IRStrobeInfo[pindex].value     = value;
    IRStrobeInfo[pindex].thread    = stdCurrentThread;
    IRStrobeInfo[pindex].decrement = longLeader ? S(P_L_VAL) : S(P_S_VAL);
    IRStrobeInfo[pindex].state     = -3;
    IRStrobeInfo[pindex].atn       = False;
    
    IRPulseStart(pin,duty,False,passiveHigh);
    stdThreadSuspendSelf();
    IRPulseStop(pin);
}

