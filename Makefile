#PREFIX = /home/stef/tasks/pitchfork/toolchain/gcc-arm-none-eabi-5_3-2016q1/bin/arm-none-eabi
#PREFIX = /home/stef/tasks/pitchfork/clean/toolchain/gcc-arm-none-eabi-5_4-2016q2/bin/arm-none-eabi
PREFIX = arm-none-eabi
CC=$(PREFIX)-gcc
LD=$(PREFIX)-ld
OC=$(PREFIX)-objcopy
OD=$(PREFIX)-objdump
AS=$(PREFIX)-as

INCLUDES = -I. -Icore/ -Iusb/ -Iusb/msc -Isdio/ -Ilib/ -Icrypto/ -Iutils/ -Iiap/ \
			  -Ilib/sphincs -Ilib/newhope -Ilib/chacha20 -Ilib/xeddsa \
			  -Ilib/libsodium/src/libsodium/include/sodium/ -Ilib/libopencm3/include
LIBS = lib/libsodium/src/libsodium/.libs/libsodium.a lib/libopencm3/lib/libopencm3_stm32f2.a
CFLAGS += -mno-unaligned-access -DNDEBUG -g -Wall -Werror -Os \
	-mfix-cortex-m3-ldrd -msoft-float -mthumb -Wno-strict-aliasing \
	-fomit-frame-pointer -mthumb -mcpu=cortex-m3 $(INCLUDES) -DSTM32F2 -DHAVE_MSC \
	-fstack-protector --param=ssp-buffer-size=4 -DRAMLOAD -DCHACHA_ASM

LDFLAGS = -mthumb -mcpu=cortex-m3 -fno-common -Tmemmap -nostartfiles -Wl,--gc-sections -Wl,-z,relro

xeddsa_objs = lib/xeddsa/elligator.o lib/xeddsa/vxeddsa.o lib/xeddsa/xeddsa.o \
	lib/xeddsa/keygen.o lib/xeddsa/zeroize.o lib/xeddsa/curve_sigs.o \
	lib/xeddsa/fe_isequal.o lib/xeddsa/fe_mont_rhs.o lib/xeddsa/fe_montx_to_edy.o \
	lib/xeddsa/ge_montx_to_p3.o lib/xeddsa/ge_p3_to_montx.o lib/xeddsa/ge_scalarmult.o \
	lib/xeddsa/ge_scalarmult_cofactor.o lib/xeddsa/sc_clamp.o lib/xeddsa/sc_cmov.o \
	lib/xeddsa/sc_neg.o lib/xeddsa/fe_sqrt.o lib/xeddsa/sign_modified.o \
	lib/xeddsa/open_modified.o lib/xeddsa/vsign_modified.o lib/xeddsa/compare.o \
	lib/xeddsa/vopen_modified.o lib/xeddsa/ge_neg.o lib/xeddsa/ge_isneutral.o

curve_objs = lib/scalarmult/cortex_m0_mpy121666.o \
	lib/scalarmult/cortex_m0_reduce25519.o lib/scalarmult/mul.o \
	lib/scalarmult/scalarmult.o lib/scalarmult/sqr.o

newhope_objs = lib/newhope/newhope_asm.o lib/newhope/precomp.o lib/newhope/poly.o \
	lib/newhope/error_correction.o lib/newhope/newhope.o lib/newhope/fips202.o \
	lib/newhope/keccakf1600.o

usb_objs = usb/msc/usb_bsp.o usb/msc/usb_dcd.o usb/msc/usbd_core.o usb/msc/usbd_ioreq.o \
   usb/msc/usbd_msc_core.o usb/msc/usbd_msc_scsi.o usb/msc/usbd_storage_msd.o \
	usb/msc/usb_core.o usb/msc/usb_dcd_int.o usb/msc/usbd_desc.o usb/msc/usbd_msc_bot.o \
	usb/msc/usbd_msc_data.o usb/msc/usbd_req.o usb/msc/usbd_usr.o

sphincs_objs = lib/sphincs/crypto_stream_chacha20.o lib/sphincs/chacha.o \
	lib/sphincs/wots.o lib/sphincs/prg.o lib/sphincs/hash.o \
	lib/sphincs/horst.o lib/sphincs/sign.o

util_objs = utils/memmove.o utils/strlen.o utils/memcpy.o utils/memset.o utils/memcmp.o \
	utils/pgpwords_data.o utils/pgpwords.o utils/lzg/decode.o utils/lzg/checksum.o \
	utils/abort.o utils/qrcode.o utils/widgets.o utils/itoa.o utils/ntohex.o utils/utils.o

objs = core/oled.o crypto/kex.o main.o core/rng.o core/adc.o core/ssp.o \
	core/clock.o core/systimer.o core/mpu.o core/init.o core/usb.o core/irq.o \
	core/dma.o sdio/sdio.o sdio/sd.o core/led.o core/keys.o core/delay.o core/xentropy.o \
	core/startup.o usb/dual.o crypto/mixer.o crypto/master.o crypto/randombytes_pitchfork.o \
	crypto/pbkdf2_generichash.o crypto/axolotl.o core/smallfonts.o core/stfs.o core/user.o \
	crypto/fwsig.o crypto/browser.o core/nrf.o crypto/pf_store.o \
	$(usb_objs) $(xeddsa_objs) $(curve_objs) $(newhope_objs) $(sphincs_objs)\
	$(util_objs) \
	iap/fwupdater.lzg.o crypto/pitchfork.o # keep these last

all : main.bin signer/signer tools/store-master-key.bin tools/lock-flash.bin

full: clean all doc main.check tags

upload: main.bin
	dfu-util -d 0483:df11 -c 1 -i 0 -a 0 -s 0x08000000 -D main.bin

unsigned.main.elf: $(objs) memmap
	$(CC) $(LDFLAGS) -o unsigned.main.elf $(objs) $(LIBS)

main.unsigned.bin: unsigned.main.elf
	$(OC) --gap-fill 0xff unsigned.main.elf main.unsigned.bin -O binary

signer/signer:
	cd signer; make

signature.o: signer/signer main.unsigned.bin # TODO handle signer/master.key
	signer/signer signer/master.key main.unsigned.bin >signature.bin
	$(OC) --input-target binary --output-target elf32-littlearm \
			--rename-section .data=.sigSection \
	      --binary-architecture arm signature.bin signature.o

main.elf: main.unsigned.bin signature.o memmap $(objs)
	$(CC) $(LDFLAGS) -o main.elf $(objs) signature.o $(LIBS)

main.bin : main.elf
	$(OC) --gap-fill 0xff main.elf main.bin -O binary

main.list: main.elf
	$(OD) -Dl main.elf > main.list

iap/fwupdater.lzg.o:
	cd iap; make

tools/store-master-key.bin: tools/store-key.c main.syms
	$(CC) -mthumb -mcpu=cortex-m3 -fno-common -Ttools/store-key-memmap \
			-DSTM32F2 -Wl,--just-symbols=main.syms \
	      -Icore -Ilib/libopencm3/include -nostartfiles -Wl,--gc-sections -Wl,-z,relro \
	      -o tools/store-key.elf tools/store-key.c lib/libopencm3/lib/libopencm3_stm32f2.a
	$(OC) --gap-fill 0xff tools/store-key.elf tools/store-master-key.bin -O binary
	$(OD) -Dl tools/store-key.elf > tools/store-key.list

tools/lock-flash.elf: tools/lock-flash.c main.syms
	$(CC) -mthumb -mcpu=cortex-m3 -fno-common -Ttools/memmap \
			-DSTM32F2 -Wl,--just-symbols=main.syms \
	      -Icore -Ilib/libopencm3/include -nostartfiles -Wl,--gc-sections -Wl,-z,relro \
	      -o tools/lock-flash.elf tools/lock-flash.c lib/libopencm3/lib/libopencm3_stm32f2.a
	#$(OC) --gap-fill 0xff tools/lock-flash.elf tools/lock-flash.bin -O binary
	$(OD) -Dl tools/lock-flash.elf > tools/lock-flash.list

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

utils/pgpwords_data.o: tools/pgpwords.py
	python tools/pgpwords.py >utils/pgpwords_data.0offset
	./lib/liblzg/src/tools/lzg -9 utils/pgpwords_data.0offset utils/pgpwords_data.lzg
	$(OC) --input-target binary --output-target elf32-littlearm \
			--rename-section .data=.rodata \
	      --binary-architecture arm utils/pgpwords_data.lzg utils/pgpwords_data.o

%.o: %.S
	$(CC) $(CFLAGS) -o $@ -c $<

%.bin: %.elf
	$(OC) --gap-fill 0xff $< $@ -O binary

clean:
	rm -f *.bin $(objs) *.elf *.list signer/signer signer/*.o tools/*.bin tools/*.elf tools/*.list || true
	cd iap; make clean

clean-all: clean
	rm -rf doc/latex doc/html || true
	rm -f GPATH GRTAGS GSYMS GTAGS || true

unsigned.main.clean:
	rm $(objs)

.PHONY: clean clean-all upload full doc tags static_check unsigned.main.clean
