OBJECTS = lcd_modified.o  \
          uart_modified.o  \
          eeprom.o \
          stdInterrupts.o \
	  stdThreads.o \
	  stdQueues.o \
	  stdADC.o

libthreads.a : $(OBJECTS) 
	avr-ar -cr $@ $^ 

include ../../Makefile.inc
