#include "eeprom.h"
#include "stdInterrupts.h" 


void EEPROM_write( uInt16 address, uInt8 value )
{
    stdXDisableInterrupts();
    
    while (EECR & (1<<EEPE)) {}
    
    EEAR = address;
    EEDR = value;
    
    EECR |= (1<<EEMPE);
    EECR |= (1<<EEPE);
    
    stdXEnableInterrupts();
}

uInt8 EEPROM_read( uInt16 address )
{
    stdXDisableInterrupts();
    
    while (EECR & (1<<EEPE)) {}
    
    EEAR = address;
    
    EECR |= (1<<EERE);
    
    uInt8 result = EEDR;
    
    stdXEnableInterrupts();
    
    return result;
}

void EEPROM_copy_to( uInt16 to, Pointer from, uInt16 n )
{
    uInt8 *f= from;

    while (n--) {
        EEPROM_write( to++, *(f++) );
    }
}

void EEPROM_copy_from( Pointer to, uInt16 from, uInt16 n )
{
    uInt8 *t= to;

    while (n--) {
        *(t++)= EEPROM_read( from++ );
    }
}

