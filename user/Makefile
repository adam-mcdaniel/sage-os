CROSS_COMPILE?=riscv64-unknown-linux-gnu-
CC=$(CROSS_COMPILE)gcc
CXX=$(CROSS_COMPILE)g++
OBJCOPY=$(CROSS_COMPILE)objcopy
# Get the optimization level from the environment `OPT_LEVEL=1`, or default to -O3
OPT_LEVEL?=s
CFLAGS=-Wall -Wextra -pedantic -Werror -std=c2x -march=rv64gc -mabi=lp64d -ffreestanding -nostdlib -nostartfiles -O$(OPT_LEVEL) -Ilibc/ -mcmodel=medany
LDFLAGS=-Tlds/libc.lds -Llibc
LIBS=-lc
SOURCES=$(wildcard *.c)
OBJECTS=$(patsubst %.c,%.elf,$(SOURCES))
OBJDUMP=$(CROSS_COMPILE)objdump
SYMS=$(patsubst %.c,%.sym,$(SOURCES))
DBG=$(patsubst %.c,%.dbg,$(SOURCES))

all: $(OBJECTS) $(SYMS) $(DBG)

%.elf: %.c Makefile libc/libc.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

%.sym: %.elf
	$(OBJDUMP) -t $< > $@

%.dbg: %.elf
	$(OBJDUMP) -d $< > $@

libc/libc.a:
	$(MAKE) -C libc


.PHONY: clean
	
clean:
	$(MAKE) -C libc clean
	rm -f *.elf

