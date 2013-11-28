ARMGNU = /home/stef/tasks/cdong/stm32/gcc-arm-none-eabi-4_7-2013q3/bin/arm-none-eabi

COPS = -Wall -Werror -Os -nostdlib -nostartfiles -ffreestanding -march=armv7 -mfix-cortex-m3-ldrd -msoft-float -mthumb -Ilib/libsodium/src/libsodium/include/sodium/ -Ilib/libopencm3/include
LIBS = lib/libsodium/src/libsodium/.libs/libsodium.a lib/libopencm3_stm32f2.a

all : main.gcc.thumb.bin

upload: main.gcc.thumb.bin
	dfu-util -d 0483:df11 -c 1 -i 0 -a 0 -s 0x08000000 -D main.gcc.thumb.bin

mainobjs = uart.gcc.thumb.o rng.gcc.thumb.o adc.gcc.thumb.o \
	clock.gcc.thumb.o systimer.gcc.thumb.o haveged.gcc.thumb.o \
	randombytes_salsa20_random.gcc.thumb.o init.gcc.thumb.o \
	cdcacm.gcc.thumb.o

main.gcc.thumb.o : main.c
	$(ARMGNU)-gcc $(COPS) -c main.c -o main.gcc.thumb.o

main.gcc.thumb.bin : memmap utils.o main.gcc.thumb.o $(mainobjs) memset.gcc.thumb.o memcpy.o
	$(ARMGNU)-gcc -mthumb -march=armv7 -msoft-float -nostartfiles -ffreestanding -o main.gcc.thumb.elf -T memmap utils.o main.gcc.thumb.o $(mainobjs) $(LIBS) memcpy.o memset.gcc.thumb.o
	$(ARMGNU)-objdump -D main.gcc.thumb.elf > main.gcc.thumb.list
	$(ARMGNU)-objcopy main.gcc.thumb.elf main.gcc.thumb.bin -O binary

memcpy.o : memcpy.S
	$(ARMGNU)-as memcpy.S -o memcpy.o

utils.o : utils.s
	$(ARMGNU)-as utils.s -o utils.o

%.gcc.thumb.o: %.c
	$(ARMGNU)-gcc $(COPS) -c $< -o $@

clean:
	rm -f *.bin
	rm -f *.o
	rm -f *.elf
	rm -f *.list
	rm -f *.bc
	rm -f *.opt.s
	rm -f *.norm.s
