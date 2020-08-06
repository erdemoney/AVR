
clean :
	make -C Lib/threads                    local_clean
	make -C Lib/IR                         local_clean
	make -C Demo/tasksDemo                 local_clean
	make -C Demo/dacTest                   local_clean
	make -C Demo/fuelMeterTest             local_clean
	make -C Demo/timeCtxSwitch             local_clean
	make -C Demo/uartSanityTest            local_clean
	make -C Demo/sanityTest                local_clean
	make -C Demo/ledigits                  local_clean
	make -C Demo/irqDetect                 local_clean
	make -C Demo/acousticPing              local_clean
	make -C Demo/TWITest                   local_clean
	make -C Projects/LEDDisplayDriverDemo  local_clean
	make -C Projects/IRbeam                local_clean
	make -C Projects/ledArray              local_clean
	make -C Projects/shiftRegister         local_clean
	make -C Projects/IRreceiver            local_clean
	make -C Projects/IRfrustrator          local_clean
	make -C Projects/capacitiveSensing     local_clean
