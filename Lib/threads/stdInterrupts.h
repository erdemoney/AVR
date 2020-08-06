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

#ifndef stdInterrupts_INCLUDED
#define stdInterrupts_INCLUDED

/*-------------------------------- Includes ---------------------------------*/

#include "stdTypes.h"
#include <avr/pgmspace.h>
#include <avr/interrupt.h>


#ifdef __cplusplus
extern "C" {
#endif

/*-------------------- Interrupt Enabling/Disabling Macros -------------------*/

/*
 * Saved interrupts type.
 */
typedef Byte   stdIFlags;


/*
 * Function        : Enable all interrupts.
 * Parameters      : 
 */
void stdXEnableInterrupts();

#define stdXEnableInterrupts() \
   {  __asm__ __volatile__ ("sei" ::: "memory"); }


/*
 * Function        : Disable all interrupts.
 * Parameters      : 
 */
void stdXDisableInterrupts();

#define stdXDisableInterrupts() \
   { __asm__ __volatile__ ("cli" ::: "memory"); }


/*
 * Function        : Enable all interrupts.
 *                   The old interrupt enabling state is saved
 *                   into the 32 bit integer location specified as
 *                   argument.
 * Parameters      : interrupts   (O) State save variable.
 */
void stdEnableInterrupts(stdIFlags *interrupts);

#define stdEnableInterrupts(interrupts) \
   {                             \
      *(interrupts)= SREG&0x80;  \
       stdXEnableInterrupts();   \
   }


/*
 * Function        : Disable all interrupts that are not NMI.
 *                   The old interrupt enabling state is saved
 *                   into the 32 bit integer location specified as
 *                   argument. 
 * Parameters      : interrupts   (O) State save variable.
 */
void stdDisableInterrupts(stdIFlags *interrupts);

#define stdDisableInterrupts(interrupts) \
   {                               \
      *(interrupts)= SREG&0x80;    \
       stdXDisableInterrupts();    \
   }


/*
 * Function        : Restore interrupt enabling state.
 *                   This macro restores the state as saved by
 *                   stdDisableInterrupts or stdEnableInterrupts. 
 * Parameters      : interrupts   (I) Interrupt enabling state to restore.
 */
void stdRestoreInterrupts(stdIFlags interrupts);

#define stdRestoreInterrupts(interrupts) \
   { SREG = (SREG & ~0x80) | interrupts; }


/*
 * Function        : Run interrupt handler action function from initial ISR.
 *                   This function will run the specified function on a shared
 *                   interrupt stack, and afterwards call the scheduler to check
 *                   on newly acivated threads.
 * Parameters      : isr   (I) Interrupt handler action function.
 */
void stdRunISR( void (*isr)() );


/*
 * Function        : Lock/unlock the thread scheduler around parts of an interrupt
 *                   handler that may ready new threads. These functions are shortcuts
 *                   around stdRunISR. 
 */
    /*
     * Hidden imports from stdThreads
     * for interrupt handler locking:
     */
    extern Bool stdSchedLock;
    void stdReschedule();

#define stdISRLock() \
{                           \
    stdSchedLock = True;    \
}

#define stdISRUnLock() \
{                           \
    stdSchedLock = False;   \
                            \
    if (stdRunQ) {          \
        stdReschedule();    \
    }                       \
}

#ifdef __cplusplus
}
#endif

#endif




