BP=..
objs = sc_clamp.o xeddsa.o sign_modified.o open_modified.o fe_montx_to_edy.o zeroize.o sc_neg.o sc_cmov.o fe_1.o fe_add.o fe_mul.o fe_sub.o


all: signer

signer: $(objs)
	gcc -o signer signer.c $(objs) -I$(BP)/lib/xeddsa -I/usr/include/sodium /usr/lib/libsodium.a

xeddsa.o: $(BP)/lib/xeddsa/xeddsa.c
	gcc -I$(BP)/lib/xeddsa -I/usr/include/sodium -o $@ -c $<

sc_clamp.o: $(BP)/lib/xeddsa/sc_clamp.c
	gcc -I$(BP)/lib/xeddsa -o $@ -c $<

sc_cmov.o: $(BP)/lib/xeddsa/sc_cmov.c
	gcc -I$(BP)/lib/xeddsa -o $@ -c $<

sc_neg.o: $(BP)/lib/xeddsa/sc_neg.c
	gcc -I$(BP)/lib/xeddsa -o $@ -c $<

zeroize.o: $(BP)/lib/xeddsa/zeroize.c
	gcc -I$(BP)/lib/xeddsa -o $@ -c $<

fe_montx_to_edy.o: $(BP)/lib/xeddsa/fe_montx_to_edy.c
	gcc -I$(BP)/lib/xeddsa -o $@ -c $<

open_modified.o: $(BP)/lib/xeddsa/open_modified.c
	gcc -I$(BP)/lib/xeddsa -I/usr/include/sodium -o $@ -c $<

sign_modified.o: $(BP)/lib/xeddsa/sign_modified.c
	gcc -I$(BP)/lib/xeddsa -I/usr/include/sodium -o $@ -c $<

fe_1.o: $(BP)/lib/xeddsa/fe_1.c
	gcc -I$(BP)/lib/xeddsa -o $@ -c $<

fe_add.o: $(BP)/lib/xeddsa/fe_add.c
	gcc -I$(BP)/lib/xeddsa -o $@ -c $<

fe_mul.o: $(BP)/lib/xeddsa/fe_mul.c
	gcc -I$(BP)/lib/xeddsa -o $@ -c $<

fe_sub.o: $(BP)/lib/xeddsa/fe_sub.c
	gcc -I$(BP)/lib/xeddsa -o $@ -c $<
