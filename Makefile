PREFIX = /home/stef/tasks/cdong/stm32/gcc-arm-none-eabi-4_7-2013q3/bin/arm-none-eabi
CC=$(PREFIX)-gcc
LD=$(PREFIX)-ld
OC=$(PREFIX)-objcopy
OD=$(PREFIX)-objdump
AS=$(PREFIX)-as

INCLUDES = -I. -Icore/ -Iusb/ -Icrypto/ -Iutils/ -Ilib/libsodium/src/libsodium/include/sodium/ -Ilib/libopencm3/include
LIBS = lib/libsodium/src/libsodium/.libs/libsodium.a lib/libopencm3_stm32f2.a
CFLAGS = -Wall -Werror -Os -mfix-cortex-m3-ldrd -msoft-float -mthumb -Wno-strict-aliasing -mcpu=cortex-m3 $(INCLUDES)
LDFLAGS = $(LIBS) -fno-common -Tmemmap -nostartfiles

mainobjs = utils/utils.o main.o core/uart.o core/rng.o core/adc.o \
	core/clock.o core/systimer.o crypto/mixer.o cmd.o core/init.o \
	crypto/randombytes_salsa20_random.o usb/cdcacm.o \
	core/dma.o sdio/sdio.o sdio/sd.o utils/memcpy.o utils/memset.o

all : main.bin

upload: main.bin
	dfu-util -d 0483:df11 -c 1 -i 0 -a 0 -s 0x08000000 -D main.bin

main.bin : memmap $(mainobjs)
	$(CC) -o main.elf $(mainobjs) $(LDFLAGS)
	$(OD) -D main.elf > main.list
	$(OC) main.elf main.bin -O binary
	gtags

clean:
	rm -f *.bin
	rm -f $(mainobjs)
	rm -f *.elf
	rm -f *.list
	rm -f GPATH GRTAGS GSYMS GTAGS

.PHONY: clean upload
