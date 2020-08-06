
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "uart.h"

#include "stdThreads.h" 
#include "stdDefs.h" 


void ticker1F(); 

stdInstantiateThread( ticker1,     80,  ticker1F,  0, 1,   NULL     );
stdInstantiateThread( mainThread,   1,  Null,     10, 1,   &ticker1 );

/*
 * Run Queue Initialization:
 */
stdThread_t    stdCurrentThread   = &mainThread;
ThreadPrioQ_t  stdRunQ            = &mainThread;


////////////////////////////////////////////////////////////////////////////////////////////

void ticker1F()
{
    DDRB |= (1<<PB0);
    
    uInt16 nextTime = stdTime();

    while (True) {
        nextTime += stdSECOND;
        stdThreadSleep( nextTime - stdTime() );
        PORTB ^= (1<<PB0);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////

#define TWCRV_TWSTA   (1<<TWSTA)
#define TWCRV_TWSTO   (1<<TWSTO)
#define TWCRV_TWINT   (1<<TWINT)
#define TWCRV_TWEA    (1<<TWEA)

#define TWCR_VAL_COMMON    ((1<<TWEN) | (1<<TWIE) | (1<<TWINT))

#define TWCR_VAL0()        ( TWCR_VAL_COMMON )
#define TWCR_VAL1(x)       ( TWCR_VAL_COMMON | (TWCRV_##x) )
#define TWCR_VAL2(x,y)     ( TWCR_VAL_COMMON | (TWCRV_##x) | (TWCRV_##y) )
#define TWCR_VAL3(x,y,z)   ( TWCR_VAL_COMMON | (TWCRV_##x) | (TWCRV_##y) | (TWCRV_##z) )

#define TWCR_VAL1x(x)      ( TWCR_VAL_COMMON | (x) )
#define TWCR_VAL2x(x,y)    ( TWCR_VAL_COMMON | (x)         | (TWCRV_##y) )

#define TWI_ERROR(x)   


typedef void  (*twiReceiveFun_t)( uInt8 value );
typedef uInt8 (*twiReplyFun_t  )( );

static uInt8             twiAddress_RW;
static uInt8             twiSendValue;
static uInt8             twiReceiveValue;
static stdThread_t       twiThread;
static twiReceiveFun_t   twiReceiveFun;
static twiReplyFun_t     twiReplyFun;

static void twiHandler()
{
    uint8_t status = TWSR & ~7;

    switch (status) {
   /*
    * General :
    */
    case 0x08 : // START has been transmitted
    case 0x10 : // Repeated START has been transmitted
        TWDR = twiAddress_RW;
        TWCR = TWCR_VAL1x( TWCR & (1<<TWEA) );
        break;
    
    case 0x38 : // Arbitration Lost
        // Retry
        TWCR = TWCR_VAL2x( TWCR & (1<<TWEA), TWSTA );    
        break;


   /*
    * Master Transmitter Mode
    */
    case 0x18 : // SLA+W transmitted;                                         ACK  received
        TWDR = twiSendValue;
        TWCR = TWCR_VAL0();    
        break;

    case 0x20 : // SLA+W transmitted;                                         NACK received
        TWI_ERROR("Slave always acks in this implementation");
        break;
    
    case 0x28 : // Data byte transmitted;                                     ACK  received
        TWI_ERROR("Single byte protocol in this implementation");
        break;
    
    case 0x30 : // Data byte transmitted;                                     NACK received
        // Stop transmission; single byte transfer only
        TWCR = TWCR_VAL2( TWEA, TWSTO );    
        stdThreadResume(twiThread);
        break;
 
    
   /*
    * Slave Receiver Mode
    */
    case 0x60 : // Own SLA+W received;                                        ACK  returned
        // receive data byte, and return NACK (single byte mode)
        TWCR = TWCR_VAL1x( TWCR & (1<<TWSTA) );    
        break;

    case 0x68 : // Arbitration lost in SLA+R/W as master; own SLA+W received; ACK  returned
        TWCR = TWCR_VAL1( TWSTA );    
        break;

    case 0x70 : // GENADDR received;                                          ACK  returned
    case 0x78 : // Arbitration lost in SLA+R/W as master; GENADDR received;   ACK  returned
        TWI_ERROR("GENADDR not supported by this implementation");
        break;
    
    case 0x80 : // Data byte received to own address                          ACK  returned
        TWI_ERROR("Single byte protocol in this implementation");
        break;
        
    case 0x88 : // Data byte received to own address                          NACK returned
      { uint8_t twdr = TWDR;
        TWCR = TWCR_VAL2x( TWCR & (1<<TWSTA), TWEA );    
        twiReceiveFun( twdr );
        break;
      }
    case 0x90 : // Data byte received to GENADDR                              ACK  returned
    case 0x98 : // Data byte received to GENADDR                              NACK returned
        TWI_ERROR("GENADDR not supported by this implementation");
        break;
    
    case 0xa0 : // STOP or repeated START received
        TWCR = TWCR_VAL2x(  TWCR & (1<<TWSTA), TWEA );    
        break;

    
   /*
    * Master Receiver Mode
    */
    case 0x40 : // SLA+R transmitted;                                         ACK  received
        // receive data byte, and return NACK (single byte mode)
        TWCR = TWCR_VAL0();    
        break;
        
    case 0x48 : // SLA+R transmitted;                                         NACK received
        TWI_ERROR("Slave always acks in this implementation");
        break;

    case 0x50 : // Data byte received;                                        ACK  returned
        TWI_ERROR("Single byte protocol in this implementation");
        break;

    case 0x58 : // Data byte received;                                        NACK returned
        // Receive byte, and stop transmission; single byte transfer only
        twiReceiveValue = TWDR;
        TWCR = TWCR_VAL2( TWEA, TWSTO );    
        stdThreadResume(twiThread);
        break;
   
    
   /*
    * Slave Transmitter Mode
    */
    case 0xa8 : // Own SLA+R received;                                        ACK  returned
        TWDR = twiReplyFun();
        TWCR = TWCR_VAL1x( TWCR & (1<<TWSTA) );    
        break;
    
    case 0xb0 : // Arbitration lost in SLA+R/W as master; own SLA+R received; ACK  returned
        TWDR = twiReplyFun();
        TWCR = TWCR_VAL1( TWSTA );    
        break;
    
    case 0xb8 : // Data byte transmitted;                                     ACK  received
        TWI_ERROR("Single byte protocol in this implementation");
        break;
        
    case 0xc0 : // Data byte transmitted;                                     NACK received
        TWCR = TWCR_VAL2x( TWCR & (1<<TWSTA), TWEA );    
        break;    
    
    case 0xc8 : // Last data byte txed'                                       ACK  received (???)
        TWCR = TWCR_VAL2x( TWCR & (1<<TWSTA), TWEA );    
        break;
    
    
    default   : 
        TWI_ERROR("Unimplemented");
        break;
    }
}


void twiSend( uInt8 address, uInt8 value )
{
    twiAddress_RW = (address)<<1;
    twiSendValue  = value;
    twiThread     = stdCurrentThread;
    
    stdXDisableInterrupts();
    {
        TWCR = TWCR_VAL2x( TWCR & (1<<TWEA), TWSTA );
    }
    stdXEnableInterrupts();
    
    stdThreadSuspendSelf();
}


uInt8 twiReceive( uInt8 address )
{
    twiAddress_RW = (address<<1) + 1;
    twiThread     = stdCurrentThread;
    
    stdXDisableInterrupts();
    {
        TWCR = TWCR_VAL2x( TWCR & (1<<TWEA), TWSTA );
    }
    stdXEnableInterrupts();
    
    stdThreadSuspendSelf();
    
    return twiReceiveValue;
}




void twiSetup( uInt8 address, twiReceiveFun_t receiveFun, twiReplyFun_t replyFun )
{
    twiReceiveFun = receiveFun;
    twiReplyFun   = replyFun;

    PRR &= ~(1<<PRTWI);
    
    TWAR = address << 1;
    TWBR = 255;                                  // TW Bit Rate 255 |
    TWSR = (1<<TWPS1) | (1<<TWPS0);              // Prescale 64    -+-->   SCL freq about 1/2 kHz
    TWCR = TWCR_VAL1( TWEA );    
}

////////////////////////////////////////////////////////////////////////////////////////////



SIGNAL(TWI_vect) {
    stdRunISR(twiHandler); 
}


#define TWI_RECEIVER   1
#define TWI_SENDER     2

#define OUTPUTPINS    ((1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3))

static uint8_t me_address;
static uint8_t him_address;


static void twiRec( uInt8 value )
{ 
    PORTC &= ~OUTPUTPINS; 
    PORTC |=  OUTPUTPINS & value; 
}

static uInt8 repVal = 10;
static uInt8 twiRep()
{ 
    if (me_address == 2) {
        repVal = ~repVal;
    } else {
        repVal++;
    }
    return repVal;
}


int main()
{    
    stdSetup();
    
    DDRC |= OUTPUTPINS;

    uint8_t pd7;
    pd7        = (PIND & (1<<PD7)) != 0;
    me_address = 2+pd7;
    him_address= me_address^1;
       
    twiSetup(me_address, twiRec, twiRep);
            
    stdThreadSleep( stdSECOND );
        
    while (True) {
        stdThreadSleep( (stdSECOND/8 + 1) * me_address );
        
        twiSend( him_address, twiReceive(him_address) );
    }    
    
    return 0;
}
 
