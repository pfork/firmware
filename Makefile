#PREFIX = /home/stef/tasks/pitchfork/toolchain/arm/bin/arm-none-eabi
#PREFIX = /home/stef/tasks/pitchfork/gcc-arm-none-eabi-4_7-2013q3/bin/arm-none-eabi
PREFIX = /home/stef/tasks/pitchfork/toolchain/gcc-arm-none-eabi-5_3-2016q1/bin/arm-none-eabi
CC=$(PREFIX)-gcc
LD=$(PREFIX)-ld
OC=$(PREFIX)-objcopy
OD=$(PREFIX)-objdump
AS=$(PREFIX)-as

INCLUDES = -I. -Icore/ -Iusb/ -Iusb/msc -Isdio/ -Icrypto/ -Iutils/ \
			  -Ilib/sphincs256 -Ilib/blake -Ilib/newhope -Ilib/chacha20 \
			  -Ilib/libsodium/src/libsodium/include/sodium/ -Ilib/libopencm3/include
LIBS = lib/libsodium/src/libsodium/.libs/libsodium.a lib/libopencm3_stm32f2.a
CFLAGS = -mno-unaligned-access -g -Wall -Werror -Os \
	-mfix-cortex-m3-ldrd -msoft-float -mthumb -Wno-strict-aliasing \
	-fomit-frame-pointer -mthumb -mcpu=cortex-m3 $(INCLUDES) -DSTM32F2 -DHAVE_MSC \
	-fstack-protector --param=ssp-buffer-size=4 -DRAMLOAD

LDFLAGS = -mthumb -mcpu=cortex-m3 -fno-common -Tmemmap -nostartfiles -Wl,--gc-sections -Wl,-z,relro

objs = utils/utils.o core/oled.o crypto/kex.o main.o core/rng.o core/adc.o core/ssp.o \
	core/clock.o core/systimer.o core/mpu.o core/init.o core/usb.o core/irq.o \
	core/dma.o sdio/sdio.o sdio/sd.o core/led.o core/keys.o core/delay.o core/xentropy.o \
	core/startup.o usb/dual.o crypto/mixer.o crypto/ecdho.o crypto/master.o core/storage.o \
	crypto/randombytes_salsa20_random.o utils/memcpy.o utils/memset.o utils/memcmp.o \
	usb/msc/usb_bsp.o usb/msc/usb_dcd.o usb/msc/usbd_core.o usb/msc/usbd_ioreq.o \
   usb/msc/usbd_msc_core.o usb/msc/usbd_msc_scsi.o usb/msc/usbd_storage_msd.o \
	usb/msc/usb_core.o usb/msc/usb_dcd_int.o usb/msc/usbd_desc.o usb/msc/usbd_msc_bot.o \
	usb/msc/usbd_msc_data.o usb/msc/usbd_req.o usb/msc/usbd_usr.o crypto/pitchfork.o \
	core/smallfonts.o utils/lzg/decode.o utils/lzg/checksum.o \
	lib/newhope/poly.o lib/newhope/ntt.o lib/newhope/precomp.o \
	lib/newhope/error_correction.o lib/newhope/newhope.o lib/newhope/reduce.o \
	lib/newhope/fips202.o lib/sphincs/crypto_stream_chacha20.o lib/sphincs/chacha.o \
	utils/widgets.o utils/itoa.o utils/flashdbg.o utils/chords.o core/nrf.o \
	utils/ntohex.o lib/scalarmult/cortex_m0_mpy121666.o \
	lib/scalarmult/cortex_m0_reduce25519.o lib/scalarmult/mul.o \
	lib/scalarmult/scalarmult.o lib/scalarmult/sqr.o

all : main.bin
full: clean main.bin doc tags

upload: main.bin
	dfu-util -d 0483:df11 -c 1 -i 0 -a 0 -s 0x08000000 -D main.bin

main.bin : memmap $(objs)
	$(CC) $(LDFLAGS) -o main.elf $(objs) $(LIBS)
	$(OD) -Dl main.elf > main.list
	$(OC) main.elf main.bin -O binary

tags:
	gtags

doc:
	doxygen doc/doxygen.cfg

main.check:
	cppcheck --enable=all $(objs:.o=.c) $(INCLUDES) 2>main.check
	flawfinder --quiet $(objs:.o=.c) >>main.check

clean:
	rm -f *.bin
	rm -f $(objs)
	rm -f *.elf
	rm -f *.list

clean-all: clean
	rm -rf doc/latex doc/html
	rm -f GPATH GRTAGS GSYMS GTAGS

.PHONY: clean clean-all upload full doc tags static_check
