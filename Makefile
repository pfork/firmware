ARMGNU = /home/stef/tasks/cdong/stm32/gcc-arm-none-eabi-4_7-2013q3/bin/arm-none-eabi

COPS = -Wall -Werror -O2 -nostdlib -nostartfiles -ffreestanding -mthumb -Ilibsodium/src/libsodium/include/sodium/
SODIUM = libsodium/src/libsodium/.libs/libsodium.a

all : main.gcc.thumb.bin

upload: main.gcc.thumb.bin
	dfu-util -d 0483:df11 -c 1 -i 0 -a 0 -s 0x08000000 -D main.gcc.thumb.bin

mainobjs = uart.gcc.thumb.o rng.gcc.thumb.o adc.gcc.thumb.o \
	clock.gcc.thumb.o systimer.gcc.thumb.o haveged.gcc.thumb.o \
	randombytes_salsa20_random.gcc.thumb.o init.gcc.thumb.o

main.gcc.thumb.o : main.c
	$(ARMGNU)-gcc $(COPS) -c main.c -o main.gcc.thumb.o
main.gcc.thumb.bin : memmap vectors-systick.o main.gcc.thumb.o $(mainobjs) memset.gcc.thumb.o memcpy.o
	$(ARMGNU)-gcc -nostartfiles -ffreestanding -o main.gcc.thumb.elf -T memmap vectors-systick.o main.gcc.thumb.o $(mainobjs) $(SODIUM) memcpy.o memset.gcc.thumb.o
	$(ARMGNU)-objdump -D main.gcc.thumb.elf > main.gcc.thumb.list
	$(ARMGNU)-objcopy main.gcc.thumb.elf main.gcc.thumb.bin -O binary

memcpy.o : memcpy.S
	$(ARMGNU)-as memcpy.S -o memcpy.o

vectors-systick.o : vectors-systick.s
	$(ARMGNU)-as vectors-systick.s -o vectors-systick.o

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
