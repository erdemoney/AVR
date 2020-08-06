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
 *         This defines pulse timing constants 
 *         for the NEC IR protocol
 */

#ifndef irTimingsIncluded
#define irTimingsIncluded

/*-------------------------------- Includes ---------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------- Values ---------------------------------*/

/*
 * Generate 38 Khz strobe of specified duration and duty cycle on pin PB2.
 * This is achieved using TIMER1, by enabling and configuring it, then 
 * waiting for the specified amount of cycles, and finally disabling it again.
 * Note that the timer needs to be disabled or otherwise the kernel sleep
 * will not use power saving sleep. So the processor will be in a mere standby
 * with timer enabled while waiting for the amount of cycles to have elapsed, 
 * while it will go in a power savings sleep while waiting in the main loop of
 * this program.
 * 
 *    When CPU freq = 14.745600 MHz, which is about 64*61 times 38 KHz, 
 *    using wraparound of 50 and a clock prescale of 8 in Fast PWM
 *    mode gives a 38 KHz strobe on pin OC1B. 
 *    So we set OCR1A for getting that frequency, and OCR1B
 *    for an appropriate duty cycle (to be determined for our IR LEDs).
 * 
 *    When CPU freq = 8 MHz, which is about 8*27 times 38 KHz, 
 *    using wraparound of 27 and a clock prescale of 8 in Fast PWM
 *    mode gives a 38 KHz strobe on pin OC1B. 
 *    So we set OCR1A for getting that frequency, and OCR1B
 *    for an appropriate duty cycle (to be determined for our IR LEDs).
 */

 #if       defined(THREADS_SYSTEM_CLOCK_FREQ_8MHz)
               #define DC_PRESCALE   26
 #elif     defined(THREADS_SYSTEM_CLOCK_FREQ_14MHz)
               #define DC_PRESCALE   49
 #else
               #error "Unsupported system frequency"
 #endif
        



 #if defined(THREADS_SYSTEM_CLOCK_FREQ_8MHz)
               #define MAX_ERROR     ((uInt16)7)

               #define P_L_VAL     287   //  9    ms
               #define P_S_VAL     143   //  4.5  ms
               #define P_1_VAL      18   //  0.56 ms
               #define P_3_VAL      55   //  1.69 ms
               #define P_LLL_VAL   662   // 21    ms

 #elif     defined(THREADS_SYSTEM_CLOCK_FREQ_14MHz)
               #define MAX_ERROR     ((uInt16)12)

               #define P_L_VAL     520   //  9    ms
               #define P_S_VAL     260   //  4.5  ms
               #define P_1_VAL      32   //  0.56 ms
               #define P_3_VAL     100   //  1.69 ms
               #define P_LLL_VAL  1200   // 21    ms
 #else
               #error "Unsupported system frequency"
 #endif




#ifdef __cplusplus
}
#endif

#endif
