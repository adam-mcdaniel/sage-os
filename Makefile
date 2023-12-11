CROSS_COMPILE?=riscv64-unknown-linux-gnu-
GDB=$(CROSS_COMPILE)gdb
CC=$(CROSS_COMPILE)gcc
CXX=$(CROSS_COMPILE)g++
OBJCOPY=$(CROSS_COMPILE)objcopy
OBJDUMP=$(CROSS_COMPILE)objdump
# Get the optimization level from the environment `OPT_LEVEL=3`, or default to -O0
OPT_LEVEL?=0
LDSCRIPT=lds/riscv.lds
CFLAGS=-g -O$(OPT_LEVEL) -Wall -Wextra -march=rv64gc -mabi=lp64d -ffreestanding -nostdlib -nostartfiles -Iutil/include -Isrc/include -Isbi/src/include -mcmodel=medany
LDFLAGS=-T$(LDSCRIPT) $(CFLAGS) -Lutil/
LIBS=-lcosc562_util
ASM_DIR=asm
SOURCE_DIR=src
OBJ_DIR=objs
DEP_DIR=deps
SOURCES=$(wildcard $(SOURCE_DIR)/*.c)
SOURCES_ASM=$(wildcard $(ASM_DIR)/*.S)
OBJECTS=$(patsubst $(SOURCE_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))
OBJECTS+= $(patsubst $(ASM_DIR)/%.S,$(OBJ_DIR)/%.o,$(SOURCES_ASM))
DEPS=$(patsubst $(SOURCE_DIR)/%.c,$(DEP_DIR)/%.d,$(SOURCES))
KERNEL=cosc562.elf
SYMS=cosc562.sym cosc562.dbg

#### QEMU STUFF
QEMU?=qemu-system-riscv64
QEMU_DEBUG_PIPE=debug.pipe
# QEMU_HARD_DRIVE_1=hdd1.dsk
QEMU_HARD_DRIVE_1=hdd1_with_exe.dsk
QEMU_HARD_DRIVE_2=hdd2.dsk
QEMU_HARD_DRIVE_3=hdd3.dsk
QEMU_BIOS=./sbi/sbi.elf
QEMU_DEBUG=guest_errors,unimp -gdb unix:$(QEMU_DEBUG_PIPE),server,nowait
QEMU_MACH=virt #,aia=aplic #,dumpdtb=dtb.dtb
QEMU_CPU=rv64 #,h=true,v=true,vext_spec=v1.0
QEMU_CPUS=4
# QEMU_MEM=4096M
QEMU_MEM=1024M
QEMU_KERNEL=$(KERNEL)
# QEMU_OPTIONS+= -trace virtio*
QEMU_OPTIONS+= -serial mon:stdio 
QEMU_DEVICES+= -device pcie-root-port,id=bridge1,multifunction=off,chassis=0,slot=1,bus=pcie.0,addr=01.0
QEMU_DEVICES+= -device pcie-root-port,id=bridge2,multifunction=off,chassis=1,slot=2,bus=pcie.0,addr=02.0
QEMU_DEVICES+= -device pcie-root-port,id=bridge3,multifunction=off,chassis=2,slot=3,bus=pcie.0,addr=03.0
QEMU_DEVICES+= -device pcie-root-port,id=bridge4,multifunction=off,chassis=3,slot=4,bus=pcie.0,addr=04.0
QEMU_DEVICES+= -device virtio-keyboard-pci,bus=pcie.0,id=keyboard
QEMU_DEVICES+= -device virtio-tablet-pci,bus=pcie.0,id=tablet
QEMU_DEVICES+= -device virtio-rng-pci-non-transitional,bus=bridge1,id=rng
QEMU_DEVICES+= -device virtio-gpu-pci,bus=bridge2,id=gpu
# Block device
QEMU_DEVICES+= -device virtio-blk-pci-non-transitional,drive=hdd1,bus=bridge3,id=blk1
QEMU_DEVICES+= -drive if=none,format=raw,file=$(QEMU_HARD_DRIVE_1),id=hdd1
QEMU_DEVICES+= -device virtio-blk-pci-non-transitional,drive=hdd2,bus=bridge3,id=blk2
QEMU_DEVICES+= -drive if=none,format=raw,file=$(QEMU_HARD_DRIVE_2),id=hdd2
QEMU_DEVICES+= -device virtio-blk-pci-non-transitional,drive=hdd3,bus=bridge3,id=blk3
QEMU_DEVICES+= -drive if=none,format=raw,file=$(QEMU_HARD_DRIVE_3),id=hdd3
# Network device (do not uncomment unless necessary)
#QEMU_DEVICES+= -device virtio-net-pci-non-transitional,netdev=net1,bus=bridge4,id=net
#QEMU_DEVICES+= -netdev user,id=net1,hostfwd=tcp::35555-:22


all: $(KERNEL) $(SYMS)

include $(wildcard $(DEP_DIR)/*.d)

rungui: $(KERNEL) sbi
	echo "Running WITH GUI. Use target run to run WITHOUT GUI."
	$(QEMU) -bios $(QEMU_BIOS) -d $(QEMU_DEBUG) -cpu $(QEMU_CPU) -machine $(QEMU_MACH) -smp $(QEMU_CPUS) -m $(QEMU_MEM) -kernel $(QEMU_KERNEL) $(QEMU_OPTIONS) -display sdl $(QEMU_DEVICES)

run: $(KERNEL) sbi
	echo "Running WITHOUT GUI. Use target rungui to run WITH GUI."
	$(QEMU) -nographic -bios $(QEMU_BIOS) -d $(QEMU_DEBUG) -cpu $(QEMU_CPU) -machine $(QEMU_MACH) -smp $(QEMU_CPUS) -m $(QEMU_MEM) -kernel $(QEMU_KERNEL) $(QEMU_OPTIONS) $(QEMU_DEVICES)

$(KERNEL): $(OBJECTS) $(LDSCRIPT)
	$(MAKE) -C util
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.c Makefile
	$(CC) -MD -MF ./deps/$*.d $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o: $(ASM_DIR)/%.S Makefile
	$(CC) $(CFLAGS) -o $@ -c $<

cosc562.dbg: $(KERNEL)
	$(OBJDUMP) -S $< > $@

cosc562.sym: $(KERNEL)
	$(OBJDUMP) -t $< | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $@

.PHONY: clean gdb run gdbrun user util sbi

gdbrun: $(KERNEL) sbi
	$(QEMU) -S -bios $(QEMU_BIOS) -d $(QEMU_DEBUG) -cpu $(QEMU_CPU) -machine $(QEMU_MACH) -smp $(QEMU_CPUS) -m $(QEMU_MEM) -kernel $(QEMU_KERNEL) $(QEMU_OPTIONS) $(QEMU_DEVICES)

gdb: $(KERNEL) sbi
	$(GDB) $< -ex "target extended-remote $(QEMU_DEBUG_PIPE)" -ex "add-symbol-file $(QEMU_BIOS)" 

user:
	$(MAKE) -C user

util:
	$(MAKE) -C util

sbi:
	$(MAKE) -C sbi

clean:
	make -C sbi clean
	make -C user clean
	make -C util clean
	rm -f $(OBJ_DIR)/*.o $(DEP_DIR)/*.d $(KERNEL) debug.pipe $(ARCHIVE) $(SYMS)

