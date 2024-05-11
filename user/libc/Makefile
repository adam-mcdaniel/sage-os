CROSS_COMPILE?=riscv64-unknown-linux-gnu-
CC=$(CROSS_COMPILE)gcc
CXX=$(CROSS_COMPILE)g++
OBJCOPY=$(CROSS_COMPILE)objcopy
CFLAGS=-Wall -Wextra -pedantic -std=c2x -march=rv64gc -mabi=lp64d -ffreestanding -nostdlib -nostartfiles -O2 -Isrc/include -I../src/include -mcmodel=medany -fPIC
SOURCES_C=$(wildcard *.c)
SOURCES_S=$(wildcard *.S)
OBJS=$(patsubst %.c,%.o,$(SOURCES_C)) $(patsubst %.S,%.o,$(SOURCES_S))
OUT=libc.a

all: $(OUT)

$(OUT): $(OBJS) Makefile
	ar rcv $(OUT) $(OBJS)

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $<

%.o: %.S Makefile
	$(CC) $(CFLAGS) -c $<


.PHONY: clean
	
clean:
	rm -f *.o *.a

