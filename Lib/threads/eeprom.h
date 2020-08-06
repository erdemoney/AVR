#ifndef __EEPROM_H
#define __EEPROM_H


#include "stdThreads.h" 
#include "stdInterrupts.h" 


void  EEPROM_write( uInt16 address, uInt8 value );
uInt8 EEPROM_read( uInt16 address );

void  EEPROM_copy_to  ( uInt16  to, Pointer from, uInt16 n );
void  EEPROM_copy_from( Pointer to, uInt16  from, uInt16 n );
 
#endif
