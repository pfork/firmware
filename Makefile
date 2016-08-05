#PREFIX = /home/stef/tasks/pitchfork/toolchain/arm/bin/arm-none-eabi
#PREFIX = /home/stef/tasks/pitchfork/gcc-arm-none-eabi-4_7-2013q3/bin/arm-none-eabi
PREFIX = /home/stef/tasks/pitchfork/toolchain/gcc-arm-none-eabi-5_3-2016q1/bin/arm-none-eabi
CC=$(PREFIX)-gcc
LD=$(PREFIX)-ld
OC=$(PREFIX)-objcopy
OD=$(PREFIX)-objdump
AS=$(PREFIX)-as

INCLUDES = -I. -Icore/ -Iusb/ -Iusb/msc -Isdio/ -Ilib/ -Icrypto/ -Iutils/ -Iiap/ \
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
	crypto/randombytes_pitchfork.o utils/memcpy.o utils/memset.o utils/memcmp.o \
	crypto/pbkdf2_generichash.o \
	usb/msc/usb_bsp.o usb/msc/usb_dcd.o usb/msc/usbd_core.o usb/msc/usbd_ioreq.o \
   usb/msc/usbd_msc_core.o usb/msc/usbd_msc_scsi.o usb/msc/usbd_storage_msd.o \
	usb/msc/usb_core.o usb/msc/usb_dcd_int.o usb/msc/usbd_desc.o usb/msc/usbd_msc_bot.o \
	usb/msc/usbd_msc_data.o usb/msc/usbd_req.o usb/msc/usbd_usr.o crypto/pitchfork.o \
	core/smallfonts.o utils/lzg/decode.o utils/lzg/checksum.o \
	utils/abort.o lib/open.o lib/blake512.o crypto/fwsig.o \
	utils/widgets.o utils/itoa.o utils/flashdbg.o utils/chords.o core/nrf.o \
	utils/ntohex.o lib/scalarmult/cortex_m0_mpy121666.o \
	lib/scalarmult/cortex_m0_reduce25519.o lib/scalarmult/mul.o \
	lib/scalarmult/scalarmult.o lib/scalarmult/sqr.o \
	lib/newhope/newhope_asm.o lib/newhope/precomp.o lib/newhope/poly.o \
	lib/newhope/error_correction.o lib/newhope/newhope.o lib/newhope/fips202.o \
	lib/newhope/keccakf1600.o lib/newhope/chacha.o lib/newhope/crypto_stream_chacha20.o \
	iap/fwupdater.lzg.o

all : main.bin signer/signer tools/store-master-key.bin

full: clean all doc main.check tags

upload: main.bin
	dfu-util -d 0483:df11 -c 1 -i 0 -a 0 -s 0x08000000 -D main.bin

main.bin : memmap $(objs) signature.o
	$(CC) $(LDFLAGS) -o main.elf $(objs) signature.o $(LIBS)
	$(OC) --gap-fill 0xff main.elf main.bin -O binary
	$(OD) -Dl main.elf > main.list

signature.o: $(objs) memmap signer/signer
	$(CC) $(LDFLAGS) -o unsigned.main.elf $(objs) $(LIBS)
	$(OC) --gap-fill 0xff unsigned.main.elf main.unsigned.bin -O binary
	signer/signer signer/master.key main.unsigned.bin >signature.bin
	$(OC) --input binary --output elf32-littlearm \
			--rename-section .data=.sigSection \
	      --binary-architecture arm signature.bin signature.o
	rm unsigned.main.elf # main.unsigned.bin

signer/signer: signer/sign.o signer/blake512.o signer/signer.c
	gcc -Ilib signer/sign.o signer/blake512.o -o signer/signer signer/signer.c -I/usr/include/sodium /usr/lib/i386-linux-gnu/libsodium.a

signer/blake512.o: lib/blake512.c
	gcc -c -o signer/blake512.o lib/blake512.c

signer/sign.o: signer/sign.c
	gcc -c -Ilib/libsodium/src/libsodium/include/sodium/ -Ilib -o signer/sign.o signer/sign.c

iap/fwupdater.lzg.o:
	cd iap; make

tools/store-master-key.bin: tools/store-key.c main.syms
	$(CC) -mthumb -mcpu=cortex-m3 -fno-common -Ttools/store-key-memmap \
			-DSTM32F2 -I/lib/libopencm3/include -Wl,--just-symbols=main.syms \
	      -Icore -Ilib/libopencm3/include -nostartfiles -Wl,--gc-sections -Wl,-z,relro \
	      -o tools/store-key.elf tools/store-key.c lib/libopencm3_stm32f2.a
	$(OC) --gap-fill 0xff tools/store-key.elf tools/store-master-key.bin -O binary
	$(OD) -Dl tools/store-key.elf > tools/store-key.list

main.syms: main.elf
	nm -g main.elf | sed 's/\([^ ]*\) [^ ]* \([^ ]*\)$$/\2 = 0x\1;/' >main.syms

tags:
	gtags

doc:
	doxygen doc/doxygen.cfg

main.check:
	cppcheck --enable=all $(objs:.o=.c) $(INCLUDES) 2>main.check
	flawfinder --quiet $(objs:.o=.c) >>main.check

#%.bin: %.elf
#	$(OC) -Obinary --gap-fill 0xff $(*).elf $(*).bin
#
# %.elf: %.o
# 		$(LD) -o $(*).elf  $(*).o $(LDFLAGS) $(LIBS)
#
# %.o: %.c
# 		$(CC) $(CFLAGS) -o $@ -c $<

%.o: %.S
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f *.bin
	rm -f $(objs)
	rm -f *.elf
	rm -f *.list
	rm signer/signer signer/*.o
	rm tools/*.bin tools/*.elf tools/*.list
	cd iap; make clean

clean-all: clean
	rm -rf doc/latex doc/html
	rm -f GPATH GRTAGS GSYMS GTAGS

.PHONY: clean clean-all upload full doc tags static_check
