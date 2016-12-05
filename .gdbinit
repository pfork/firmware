#target remote | openocd -f /home/stef/tasks/cdong/stm32/openocd.cfg -c gdb_port pipe

set remote hardware-breakpoint-limit 6
set remote hardware-watchpoint-limit 4
set mem inaccessible-by-default off
set disassemble-next-line on
set output-radix 16

define hook-step
   mon cortex_m maskisr on
end
define hookpost-step
   mon cortex_m maskisr off
end

define con
  target extended-remote /dev/ttyACM0
  mon swdp_scan
  attach 1
  mon vector_catch enable hard int bus stat chk nocp mm reset
end

define noop
  tbreak $pc+2
  jump $pc+2
end

con
