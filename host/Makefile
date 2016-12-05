LDFLAGS=-lusb-1.0 -lsodium
CFLAGS=-Isphincs

sphincsobjs = sphincs/consts.o sphincs/hash.o sphincs/horst.o sphincs/permute.o sphincs/prg.o sphincs/sign.o sphincs/wots.o

all: pitchfork

pitchfork: $(sphincsobjs) pitchfork.o main.c
	$(CC) main.c -o $@ $(sphincsobjs) pitchfork.o $(LDFLAGS)

clean:
	@rm *.o $(sphincsobjs) pitchfork || true

.PHONY: clean
