# Makefile for IaiServerless Samples (Dual-Mode Build)

CC = gcc
LD = ld
OBJCOPY = objcopy

# Compiler flags for all user functions
CFLAGS = -ffreestanding -nostdlib -m64 -O2 -Wall -Wextra

# Linker flags
LDFLAGS_RAW = -Ttext 0x0
LDFLAGS_SHIM = -T $(SHIM_DIR)/link.ld

# Paths to the Shim library
SHIM_DIR = ../shim
SHIM_START = $(SHIM_DIR)/start.o
SHIM_LIB = $(SHIM_DIR)/libshim.a

# 1. Separate the C files into 'bare' and 'shim' categories
RAW_SRCS = $(wildcard bare_*.c)
SHIM_SRCS = $(filter-out $(RAW_SRCS), $(wildcard *.c))

# 2. Define the expected outputs
RAW_ELFS = $(RAW_SRCS:.c=.elf)
SHIM_ELFS = $(SHIM_SRCS:.c=.elf)
BINS = $(RAW_SRCS:.c=.bin) $(SHIM_SRCS:.c=.bin)

# Default target: build all flat binaries
all: $(BINS)

# Step 3: Strip the ELF headers to create the raw flat binaries (applies to both)
%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
	@echo "--> Successfully generated Unikernel binary: $@"

# Step 2A: Link RAW binaries (No Shim)
# This uses Static Pattern Rules to only apply to files starting with 'raw_'
$(RAW_ELFS): %.elf: %.o
	$(LD) $(LDFLAGS_RAW) $< -o $@

# Step 2B: Link SHIM binaries (With libshim.a and start.o)
$(SHIM_ELFS): %.elf: %.o
	$(LD) $(LDFLAGS_SHIM) $(SHIM_START) $< $(SHIM_LIB) -o $@

# Step 1: Compile the C source code into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up all build artifacts
clean:
	rm -f *.o *.elf *.bin
	@echo "Cleaned sample binaries."

.PHONY: all clean
