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
 *         This module implements IR remote command sending and receiving,
 *         currently NEC protocol only.
 */

#ifndef stdIRIncluded
#define stdIRIncluded

/*-------------------------------- Includes ---------------------------------*/

#include "stdTypes.h"
#include "irTimings.h"

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------- Types ----------------------------------*/

typedef enum {
    DC_100 = DC_PRESCALE,
    DC_50  = DC_100/2,
    DC_33  = DC_100/3,
    DC_25  = DC_100/4,
    DC_20  = DC_100/5,
    DC_12  = DC_100/8,
    DC_10  = DC_100/10,
    DC_8   = DC_100/12,
    DC_4   = DC_100/25,
} DutyCycle;

typedef enum {
    IR_OC0A__  = 0x0,       // PD6  DOES NOT WORK DUE TO TIMER SPECIAL CASE
    IR_OC0B    = 0x1,       // PD5
    IR_OC1A    = 0x2,       // PB1
    IR_OC1B    = 0x3,       // PB2
} IRPulseOutput;

/*-------------------------- IR Basic Pulse Sending -------------------------*/

void IRPulseStart( IRPulseOutput pin, DutyCycle duty, Bool on, Bool passiveHigh );
void IRPulseStop ( IRPulseOutput pin );

/*-------------------------------- IR Sending -------------------------------*/

void IRPulseTimer0COMPA();
void IRPulseTimer1COMPA();
void IRSendNECCode( IRPulseOutput pin, DutyCycle duty, uInt32 value, Bool longLeader, Bool passiveHigh );

/*------------------------------- IR Receiving ------------------------------*/


void IRReceiveAbort(uInt32 abortValue);
void IRRecTimer1CAPT();
uInt32 IRReceive( Bool *longLeader );


#ifdef __cplusplus
}
#endif

#endif
