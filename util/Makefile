CROSS_COMPILE?=riscv64-unknown-linux-gnu-
GDB=$(CROSS_COMPILE)gdb
CC=$(CROSS_COMPILE)gcc
CXX=$(CROSS_COMPILE)g++
OBJCOPY=$(CROSS_COMPILE)objcopy

LDSCRIPT=lds/riscv.lds
CFLAGS=-g -O0 -Wall -Wextra -pedantic -Werror -std=c2x -march=rv64gc -mabi=lp64d -ffreestanding -nostdlib -nostartfiles -Iinclude -mcmodel=medany
ASM_DIR=asm
SOURCE_DIR=src
OBJ_DIR=objs
DEP_DIR=deps
SOURCES=$(wildcard $(SOURCE_DIR)/*.c)
SOURCES_ASM=$(wildcard $(ASM_DIR)/*.S)
OBJECTS=$(patsubst $(SOURCE_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))
OBJECTS+= $(patsubst $(ASM_DIR)/%.S,$(OBJ_DIR)/%.o,$(SOURCES_ASM))
DEPS=$(patsubst $(SOURCE_DIR)/%.c,$(DEP_DIR)/%.d,$(SOURCES))
ARCHIVE=libcosc562_util.a
TEST=test

all: $(ARCHIVE) $(TEST)

include $(wildcard $(DEP_DIR)/*.d)

$(TEST): test.o $(ARCHIVE) Makefile
	$(CC) -L. -o $@ $< -lcosc562_util 

test.o: test.c Makefile
	$(CC) -Wall -Wextra -Werror -pedantic -Wno-builtin-declaration-mismatch -O0 -g -Iinclude -o $@ -c $<

$(ARCHIVE): $(OBJECTS)
	$(AR) rc $(ARCHIVE) $(OBJ_DIR)/*.o

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.c Makefile
	$(CC) -MD -MF ./deps/$*.d $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o: $(ASM_DIR)/%.S Makefile
	$(CC) $(CFLAGS) -o $@ -c $<


.PHONY: clean

clean:
	rm -f $(OBJ_DIR)/*.o $(DEP_DIR)/*.d $(ARCHIVE) test test.o

