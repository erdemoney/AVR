/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *     This API provides some general definitions.
 */

#ifndef stdDefs_INCLUDED
#define stdDefs_INCLUDED

/*-------------------------------- Includes ---------------------------------*/

#include "stdTypes.h"
#include <avr/pgmspace.h>
#include <avr/interrupt.h>


#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------- Definitions -------------------------------*/

#define COMB3(_prefix,_v2,_v1,_v0)   ((_v2 << _prefix##2) | (_v1 << _prefix##1) | (_v0 << _prefix##0) )

#define TIMER0_PRESCALE_1           COMB3(CS0,0,0,1)
#define TIMER0_PRESCALE_8           COMB3(CS0,0,1,0)
#define TIMER0_PRESCALE_64          COMB3(CS0,0,1,1)
#define TIMER0_PRESCALE_256         COMB3(CS0,1,0,0)
#define TIMER0_PRESCALE_1024        COMB3(CS0,1,0,1)
#define TIMER0_PRESCALE_T0_FALLING  COMB3(CS0,1,1,0)
#define TIMER0_PRESCALE_T0_RISING   COMB3(CS0,1,1,1)

#define TIMER1_PRESCALE_1           COMB3(CS1,0,0,1)
#define TIMER1_PRESCALE_8           COMB3(CS1,0,1,0)
#define TIMER1_PRESCALE_64          COMB3(CS1,0,1,1)
#define TIMER1_PRESCALE_256         COMB3(CS1,1,0,0)
#define TIMER1_PRESCALE_1024        COMB3(CS1,1,0,1)
#define TIMER1_PRESCALE_T1_FALLING  COMB3(CS1,1,1,0)
#define TIMER1_PRESCALE_T1_RISING   COMB3(CS1,1,1,1)

#define TIMER2_PRESCALE_1           COMB3(CS2,0,0,1)
#define TIMER2_PRESCALE_8           COMB3(CS2,0,1,0)
#define TIMER2_PRESCALE_32          COMB3(CS2,0,1,1)
#define TIMER2_PRESCALE_64          COMB3(CS2,1,0,0)
#define TIMER2_PRESCALE_128         COMB3(CS2,1,0,1)
#define TIMER2_PRESCALE_256         COMB3(CS2,1,1,0)
#define TIMER2_PRESCALE_1024        COMB3(CS2,1,1,1)
    


#ifdef __cplusplus
}
#endif

#endif




