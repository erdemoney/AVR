#
# System configuration:
#
ifndef MMCU
    MMCU             = 168
    MMCU             = 328
    MMCU             = 328p
endif


ifndef AVRPROGRAMMER
    AVRPROGRAMMER    = avr109 -P /dev/ttyUSB0         # NerdKit
    AVRPROGRAMMER    = usbtiny                        # SparkFun pocket AVR programmer  (www.sparkfun.com/products/9825)
endif


ifndef THREADS_SYSTEM_CLOCK_FREQ
    THREADS_SYSTEM_CLOCK_FREQ  = 32kHz                # 32 kHz external oscillator, use lfuse_INTERNAL_8MHz_OSCILLATOR.hex
    THREADS_SYSTEM_CLOCK_FREQ  = 14MHz                # External oscillator,        use lfuse_EXTERNAL_FULL_SWING_OSCILLATOR.hex
    THREADS_SYSTEM_CLOCK_FREQ  = 8MHz                 # Internal oscillator,        use lfuse_INTERNAL_8MHz_OSCILLATOR.hex
endif


ifndef THREADS_SYSTEM_TIMER_ASYNC
    THREADS_SYSTEM_TIMER_ASYNC = ASYNC                # Needs 32 kHz external oscillator for async TIMER2
    THREADS_SYSTEM_TIMER_ASYNC = NO_ASYNC
endif


ifndef THREADS_SYSTEM_TIMER_FREQ
    THREADS_SYSTEM_TIMER_FREQ  =  32Hz
    THREADS_SYSTEM_TIMER_FREQ  =  64Hz
    THREADS_SYSTEM_TIMER_FREQ  = 128Hz
    THREADS_SYSTEM_TIMER_FREQ  = 512Hz
    THREADS_SYSTEM_TIMER_FREQ  =  1kHz
endif



THREADS_CONFIGURATION = -DTHREADS_SYSTEM_CLOCK_FREQ_${THREADS_SYSTEM_CLOCK_FREQ} \
                        -DTHREADS_SYSTEM_TIMER_FREQ_${THREADS_SYSTEM_TIMER_FREQ} \
                        -DTHREADS_SYSTEM_TIMER_${THREADS_SYSTEM_TIMER_ASYNC}

SOURCE_TOP       = $(dir $(wildcard ../Makefile.inc ../../Makefile.inc))

IRLIB_DIR        = $(SOURCE_TOP)/Lib/IR
IRLIB            = $(IRLIB_DIR)/libIR.a

THREADSLIB_DIR   = $(SOURCE_TOP)/Lib/threads
THREADSLIB       = $(THREADSLIB_DIR)/libthreads.a

AVRDUDEFLAGS     = -c $(AVRPROGRAMMER) -p m$(MMCU) -b 115200

CFLAGS           = -g -Os -Wall -mmcu=atmega$(MMCU) -I. -I$(IRLIB_DIR) -I$(THREADSLIB_DIR) ${THREADS_CONFIGURATION}

ifeq ($(MMCU),328p)
  CFLAGS += -include ${THREADSLIB_DIR}/io_328p.h -fstack-usage
endif


run   : a.hex
	sudo avrdude ${AVRDUDEFLAGS} -U flash:w:a.hex:a

.PHONY : $(IRLIB) $(THREADSLIB) run clean

$(IRLIB)      : ; make -C $(IRLIB_DIR)
$(THREADSLIB) : ; make -C $(THREADSLIB_DIR)

%.o : %.c ${SOURCE_TOP}/Makefile.inc ; avr-gcc -c $< $(CFLAGS) -o $@

a.hex : a.out
	avr-objcopy -O ihex a.out a.hex

a.out : $(OBJECTS) $(IRLIB) $(THREADSLIB)
	avr-gcc $(CFLAGS) $^ -o $@

local_clean:
	rm -f a.hex a.out *.o *.a *.su

clean:
	cd ${SOURCE_TOP}; make clean
