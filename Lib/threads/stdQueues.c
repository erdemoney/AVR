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
 *         This module implements fixed capacity synchronous queues.
 */

/*--------------------------------- Includes --------------------------------*/

#include "stdThreads.h"

/*------------------------------ Bounded Queues -----------------------------*/


/* 
 * Function        : Put element in queue or wait.
 *                   Put element into queue; if the number of 
 *                   elements held by the queue has reached its
 *                   capacity, then wait until a slot becomes 
 *                   available. 
 * Parameters      : queue   (I) Queue to put 'element' into.
 *                   element (I) Element to queue.
 */
void stdQueuePut (stdQueue_t queue, uInt16 element)
{
    uInt16 *contents= (uInt16*)(queue+1);

    stdSemP (&queue->put);
    stdXDisableInterrupts();
    contents[(queue->last++) & queue->mask ] = element;
    stdXEnableInterrupts();
    stdSemV (&queue->get);
}



/* 
 * Function        : Put element into queue or fail.
 *                   Put element into queue, or return immediate
 *                   failure if the queue's capacity has been reached. 
 * Parameters      : queue   (I) Queue to put 'element' into.
 *                   element (I) Element to queue.
 * Function Result : True iff. operation succeeded.
 */        
Bool stdQueueTryPut (stdQueue_t queue, uInt16 element)
{
    uInt16 *contents= (uInt16*)(queue+1);

    if (stdSemTryP(&queue->put)) {
        stdXDisableInterrupts();
        contents[(queue->last++) & queue->mask ] = element;
        stdXEnableInterrupts();
        stdSemV (&queue->get);
        return True;
    } else {
        return False;
    }
}



/* 
 * Function        : Read element from queue or wait.
 *                   Read element from queue; if the queue is empty,  
 *                   then wait until an element becomes available.
 * Parameters      : queue   (I) Queue to read from.
 *                   element (O) Pointer to result location.
 */        
void stdQueueGet (stdQueue_t queue, uInt16 *element)
{
    uInt16 *contents= (uInt16*)(queue+1);

    stdSemP (&queue->get);
    stdXDisableInterrupts();
   *element= contents[ (queue->first++) & queue->mask ];
    stdXEnableInterrupts();
    stdSemV (&queue->put);
}



/* 
 * Function        : Read element from queue or fail.
 *                   Read element from queue, or return immediate
 *                   failure if the queue is empty.
 * Parameters      : queue   (I) Queue to read from.
 *                   element (O) Pointer to result location.
 * Function Result : True iff. operation succeeded.
 */        
Bool stdQueueTryGet (stdQueue_t queue, uInt16 *element)
{
    uInt16 *contents= (uInt16*)(queue+1);

    if (stdSemTryP (&queue->get)) {
        stdXDisableInterrupts();
       *element= contents[ (queue->first++) & queue->mask ];
        stdXEnableInterrupts();
        stdSemV (&queue->put);
        return True;
    } else {
        return False;
    }
}
