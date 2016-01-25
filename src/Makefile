CC=gcc

#CFLAGS=-c -Wall -std=gnu99 -D_XOPEN_SOURCE=500 -D_GNU_SOURCE -O3 -DWIRINGPI
CFLAGS=-c -Wall -std=gnu99 -D_XOPEN_SOURCE=500 -D_GNU_SOURCE -O3 -DBCM2835LIB

#LDFLAGS=-lwiringPi -lrt
LDFLAGS=-lbcm2835 -lrt -lpthread

powermeter.o: powermeter.c

mcp320x.o: mcp320x.c mcp320x.h

ring.o: ring.h ring.c

powermeter: powermeter.o mcp320x.o ring.o
	$(CC) powermeter.o mcp320x.o ring.o $(LDFLAGS) -o $@

.PHONY: clean
clean:
	rm -rf powermeter.o powermeter mcp320x.o ring.o
