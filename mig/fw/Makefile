# MicroBlaze bring-up firmware for the MIG design (no Xilinx BSP).
#
#   make            -> test.elf
#   make merge      -> merge test.elf into the bitstream (updatemem)
#
# Requires Vivado/Vitis 2025.2 (mb-gcc + updatemem). The MicroBlaze in
# top_bd.tcl is little-endian, so -mlittle-endian is mandatory.

XILINX   ?= /tools/Xilinx/2025.2
MB_BIN   ?= $(XILINX)/gnu/microblaze/lin/bin
CC       := $(MB_BIN)/mb-gcc
CFLAGS   := -mlittle-endian -O1
CRT0     := $(shell $(CC) -mlittle-endian -print-file-name=crt0.o)

IMPL_DIR ?= ../vivado/build/k7_160t_dp_mig.runs/impl_1

test.elf: startup.c main.c mb.ld
	$(CC) $(CFLAGS) -nostdlib -T mb.ld $(CRT0) startup.c main.c -lgcc -o $@

merge: test.elf
	$(XILINX)/Vivado/bin/updatemem -force \
		-meminfo $(IMPL_DIR)/top_wrapper.mmi \
		-bit     $(IMPL_DIR)/top_wrapper.bit \
		-data    test.elf \
		-proc    top_i/microblaze_0 \
		-out     $(IMPL_DIR)/top_wrapper_fw.bit

clean:
	rm -f test.elf

.PHONY: merge clean
