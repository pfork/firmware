PREFIX = /home/stef/tasks/cdong/stm32/gcc-arm-none-eabi-4_7-2013q3/bin/arm-none-eabi
CC=$(PREFIX)-gcc
LD=$(PREFIX)-ld
OC=$(PREFIX)-objcopy
OD=$(PREFIX)-objdump
AS=$(PREFIX)-as

INCLUDES = -I. -Icore/ -Iusb/ -Iusb/msc -Isdio/ -Icrypto/ -Iutils/ -Ilib/libsodium/src/libsodium/include/sodium/ -Ilib/libopencm3/include
LIBS = lib/libsodium/src/libsodium/.libs/libsodium.a lib/libopencm3_stm32f2.a
CFLAGS = -mno-unaligned-access -g -Wall -Werror -Os -mfix-cortex-m3-ldrd -msoft-float -mthumb -Wno-strict-aliasing -march=armv7 $(INCLUDES) -DUSE_CDC_UART
LDFLAGS = -mthumb -march=armv7 -fno-common -Tmemmap -nostartfiles

LITE_LIBS = lib/libopencm3_stm32f2.a
LITE_LDFLAGS = -mthumb -march=armv7 -fno-common -Tmemmap -nostartfiles

mainobjs = utils/utils.o main.o core/uart.o core/rng.o core/adc.o \
	core/clock.o core/systimer.o crypto/mixer.o cmd.o core/init.o \
	crypto/randombytes_salsa20_random.o usb/cdcacm.o core/irq.o \
	core/dma.o sdio/sdio.o sdio/sd.o utils/memcpy.o utils/memset.o

liteobjs = utils/utils.o lite.o cmd-lite.o core/uart.o core/rng.o core/adc.o \
	core/clock.o core/systimer.o core/init.o usb/cdcacm.o core/irq.o \
	core/dma.o sdio/sdio.o sdio/sd.o core/led.o core/keys.o core/delay.o \
	core/startup.o \
	usb/msc/usb_bsp.o usb/msc/usb_dcd.o usb/msc/usbd_core.o usb/msc/usbd_ioreq.o \
   usb/msc/usbd_msc_core.o usb/msc/usbd_msc_scsi.o usb/msc/usbd_storage_msd.o \
	usb/msc/usb_core.o usb/msc/usb_dcd_int.o usb/msc/usbd_desc.o usb/msc/usbd_msc_bot.o \
	usb/msc/usbd_msc_data.o usb/msc/usbd_req.o usb/msc/usbd_usr.o

all : main.bin

upload: main.bin
	dfu-util -d 0483:df11 -c 1 -i 0 -a 0 -s 0x08000000 -D main.bin

upload-lite: lite.bin
	dfu-util -d 0483:df11 -c 1 -i 0 -a 0 -s 0x08000000 -D lite.bin

main.bin : memmap $(mainobjs)
	$(CC) $(LDFLAGS) -o main.elf $(mainobjs) $(LIBS)
	$(OD) -Dl main.elf > main.list
	$(OC) main.elf main.bin -O binary
	gtags

lite.bin : memmap $(liteobjs)
	$(CC) $(LITE_LDFLAGS) -o lite.elf $(liteobjs) $(LITE_LIBS)
	$(OD) -Dl lite.elf > lite.list
	$(OC) lite.elf lite.bin -O binary
	gtags

cmd-lite.o: cmd.c
	$(CC) $(CFLAGS) -DLITE -c -o cmd-lite.o cmd.c
lite.o: lite.c
	$(CC) $(CFLAGS) -DLITE -c -o lite.o lite.c

clean:
	rm -f *.bin
	rm -f $(mainobjs) lite.o cmd-lite.o
	rm -f *.elf
	rm -f *.list
	rm -f GPATH GRTAGS GSYMS GTAGS

clean-lite:
	rm -f $(mainobjs) lite.o cmd-lite.o

.PHONY: clean upload
