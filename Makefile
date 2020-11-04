CC = gcc
CFLAGS = -ffreestanding -m32 -fno-pie
BIN = rawOS
BUILD_DIR = ./bin
RES_DIR = ./res

# List of all .c source files.
C = $(wildcard ./src/*.c) $(wildcard ./src/alloc/*.c) $(wildcard ./src/util/*.c) $(wildcard ./src/fs/*.c)
# List of all .asm source files.
ASM = $(wildcard ./src/*.asm) $(wildcard ./src/asm/*.asm)
# All .o files go to build dir.
OBJ = $(C:%.c=$(BUILD_DIR)/%.o) $(ASM:%.asm=$(BUILD_DIR)/%.o) $(BUILD_DIR)/initrd.o
# Gcc/Clang will create these .d files containing dependencies.
DEP = $(OBJ:%.o=%.d)

all: rawOS

# run rawOS, using QEMU emulator
run: all
	qemu-system-x86_64 -drive format=raw,file=bin/rawOS -m 4G

debug: all
	qemu-system-x86_64 -s -S -drive format=raw,file=bin/rawOS -m 4G

# see rawOS binary in hex
hex: all
	od -t x1 -A n bin/rawOS

# The image is composed by the boot sector and the kernel
rawOS: $(BUILD_DIR)/boot_sect.bin $(BUILD_DIR)/kernel.bin
	cat $^ > $(BUILD_DIR)/$(BIN)

# Compilation of the boot sector
$(BUILD_DIR)/boot_sect.bin: boot/boot_sect.asm
	mkdir -p $(@D)
	nasm $< -f bin -o $@

# Linkage of the kernel
$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/src/asm/kernel_entry.o $(OBJ)
	ld -o $@ -Tlink.ld $^ --oformat binary -m elf_i386 -e main
	#ld -o $@ -Ttext 0x1000 -Tdata 0x3000 $^ --oformat binary -m elf_i386 -e main

# ramdisk stuff

$(BUILD_DIR)/initrd.o: $(BUILD_DIR)/initrd.img
	objcopy -B i386 -I binary -O elf32-i386 $^ $@

$(BUILD_DIR)/initrd.img: $(BUILD_DIR)/ramdisk/writer
	$(BUILD_DIR)/ramdisk/writer $(RES_DIR)/rawos_print.rawx rawos_print.rawx $(RES_DIR)/rawos_cursor.rawx rawos_cursor.rawx $(RES_DIR)/rawos_execve.rawx rawos_execve.rawx $(RES_DIR)/rawos_fork_3.rawx rawos_fork_3.rawx $(RES_DIR)/rawos_exit.rawx rawos_exit.rawx $(RES_DIR)/rawos_files.rawx rawos_files.rawx

read_initrd: $(BUILD_DIR)/ramdisk/reader
	$(BUILD_DIR)/ramdisk/reader true

$(BUILD_DIR)/ramdisk/writer: $(BUILD_DIR)/ramdisk/writer.o
	mkdir -p $(@D)
	$(CC) $< -o $@

$(BUILD_DIR)/ramdisk/reader: $(BUILD_DIR)/ramdisk/reader.o
	mkdir -p $(@D)
	$(CC) $< -o $@

$(BUILD_DIR)/ramdisk/writer.o: ramdisk/writer.c
	mkdir -p $(@D)
	$(CC) -c $< -o $@

$(BUILD_DIR)/ramdisk/reader.o: ramdisk/reader.c
	mkdir -p $(@D)
	$(CC) -c $< -o $@

######

# Include all .d files
-include $(DEP)

# Build target for every single object file.
# The potential dependency on header files is covered
# by calling `-include $(DEP)`.
$(BUILD_DIR)/%.o : %.c
	mkdir -p $(@D)
	# The -MMD flags additionaly creates a .d file with
	# the same name as the .o file.
	$(CC) $(CFLAGS) -MMD -c $< -o $@

$(BUILD_DIR)/%.o : %.asm
	mkdir -p $(@D)
	nasm $< -f elf32 -o $@

clean:
	rm -r $(BUILD_DIR)
