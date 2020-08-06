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
 *     This API provides ADC reading functions.
 */

/*-------------------------------- Includes ---------------------------------*/

#include "stdADC.h" 
#include "stdThreads.h" 

/*-------------------------------- Functions --------------------------------*/

    static stdThread_t adcWaiter;

    static void adcHandler()
    {  
       /*
        * See the section on ADC noise
        * reduction mode when this bit
        * is enabled:
        */
        ADCSRA &= ~(1<<ADEN);
        
        stdThreadResume(adcWaiter); 
    }

    SIGNAL(ADC_vect)
    { stdRunISR(adcHandler); }

uInt16 adcRead()
{
   /*
    * Flag current thread to be
    * woken up at ADC interrupt:
    */
    adcWaiter = stdCurrentThread;

   /*
    * Fire a conversion:
    */
    ADCSRA |= (1<<ADEN);
    ADCSRA |= (1<<ADSC);
    
   /*
    * Wait until conversion is finished:
    */
    stdThreadSuspendSelf();
    
   /*
    * Read the ADC conversion result
    * Note: ADCL has te be read first.
    */
    uInt16 resultLo = ADCL;
    uInt16 resultHi = ADCH;
    
    return (resultHi << 8) + resultLo;
}

void adcSetup( uInt8 mux )
{ 
   /* 
    * Power up the ADC:
    */ 
    PRR &= ~(1<<PRADC);

   /*
    * Set Analog to Digital Converter
    * for external reference (5V), single
    * ended input ADC0:
    */
    ADMUX = mux;
    
   /*
    * Set ADC to be enabled, with a clock prescale of 1/128
    * so that the ADC clock runs at 115.2 kHz.
    * TODO: this should be depending on THREADS_SYSTEM_CLOCK_FREQ
    */
    ADCSRA = (1<<ADIE) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
    
   /*
    * Do a conversion to warm up the ADC:
    */
    adcRead();
}

void adcTerm()
{
   /* 
    * Power down the ADC:
    */ 
    PRR |= (1<<PRADC);
}
