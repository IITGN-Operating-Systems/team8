ROOT := $(shell git rev-parse --show-toplevel)

KERN := kernel
TARGET := target/aarch64-unknown-none/release/${KERN}
TEST_TARGET := aarch64-unknown-none
SDCARD ?= $(ROOT)/ext/fat32-imgs/mock1.fat32.img
OBJCPY := cargo objcopy -- --strip-all -O binary
TTY_PATH := /dev/ttyUSB0
QEMU_ARGS ?=

.PHONY: all bin elf qemu transmit objdump nm check clean install test

all: bin

bin:
	@mkdir -p build
	cargo objcopy -- --strip-all -O binary build/$(KERN).bin

check:
	@cargo check

elf:
	@cargo build --release
	@mkdir -p build
	@cp -f $(TARGET) build/$(KERN).elf

qemu-n: bin
	./qemu.sh build/$(KERN).bin
	
qemu: bin
	./qemu.sh build/$(KERN).bin -drive file=$(SDCARD),format=raw,if=sd $(QEMU_ARGS)

qemu-gdb: bin
	./qemu.sh build/$(KERN).bin -drive file=$(SDCARD),format=raw,if=sd -s -S

qemu_screen: bin
	./qemu_screenable.sh build/$(KERN).bin -drive file=$(SDCARD),format=raw,if=sd $(QEMU_ARGS)

transmit: bin
	@echo "+ Transmitting build/$(KERN).bin to $(TTY_PATH)"
	ttywrite -i build/$(KERN).bin $(TTY_PATH)
	screen $(TTY_PATH) 115200

objdump:
	cargo objdump --release -- --disassemble --no-show-raw-insn

nm: elf
	cargo nm build/$(KERN).elf

clean:
	cargo clean
	rm -rf build

install: bin
	cp ./build/$(KERN).bin /Volumes/boot/kernel8.img
	diskutil eject disk2	

test:
	cargo test -Z build-std --target=$(TEST_TARGET)
