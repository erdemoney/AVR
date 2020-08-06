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
 *     This API provides macros enabling/disabling the AVR interrupts.
 */

/*-------------------------------- Includes ---------------------------------*/

#include "stdInterrupts.h"
#include "stdThreads.h"

/*-------------------------------- Functions --------------------------------*/

/*
 * ISR call stack:
 *
 *  NB: not currently used; must implement this module in assembly,
 *      or else do the stack switches in inline assembly and check
 *      that the currently used compiler does not generate code that
 *      tries to access the stack frame when the stack is switched.
 *
 *      A shared interrupt stack is only useful when complex interrupt
 *      service routines are used; otherwise, stack switching just costs
 *      about 10 additional cycles of overhead.
 */
#define ISR_STACK_SIZE     50

//uInt8 stdSharedIsrStack[ISR_STACK_SIZE];


/*
 * Function        : Run interrupt handler action function from initial ISR.
 *                   This function will run the specified function on a shared
 *                   interrupt stack, and afterwards call the scheduler to check
 *                   on newly acivated threads.
 * Parameters      : isr   (I) Interrupt handler action function.
 */
void stdRunISR( void (*isr)() )
{
   /*
    * Prevent context switches when the isr
    * readies threads. This would be *very* bad:
    */
    stdSchedLock = True;
    
   /*
    * switch here to shared interrupt stack
    */
    //HERE
     
    isr();
    
   /*
    * switch back to original stack
    * (whick is the stack of the interrupted thread):
    */
    //HERE

   /*
    * Reenable scheduling, and then call the scheduler
    * so that it can switch in newly readied threads 
    * in case these have higher priority than the 
    * current (interrupted) thread:
    */
    stdSchedLock = False;
    
    if (stdRunQ) {
        stdReschedule();
    }
    
   /*
    * Prevent race conditions due to interrupts occurring
    * immediately before execution of a sleep instruction that
    * is intended to wait for this interrupt. 
    * See also function SLEEP in stdThreads.c.
    */
    SMCR = 0;
}



