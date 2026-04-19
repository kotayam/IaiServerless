# Makefile for IaiServerless Samples (Dual-Mode Build)

CC = gcc
LD = ld
OBJCOPY = objcopy

# Compiler flags for all user functions
CFLAGS = -ffreestanding -m64 -O2 -Wall -Wextra

# Linker flags
LDFLAGS_RAW = -Ttext 0x0
LDFLAGS_SHIM = -T $(SHIM_DIR)/link.ld

# Paths to the Shim library
SHIM_DIR = ../../shim
SHIM_START = $(SHIM_DIR)/start.o
SHIM_LIB = $(SHIM_DIR)/libshim.a

# 1. Separate the C files into 'bare' and 'shim' categories
RAW_SRCS = $(wildcard bare_*.c)
SHIM_SRCS = $(filter-out $(RAW_SRCS) iai.c timing.c, $(wildcard *.c))

# 2. Define the expected outputs
RAW_ELFS = $(RAW_SRCS:.c=.elf)
SHIM_ELFS = $(SHIM_SRCS:.c=.elf)
BINS = $(RAW_SRCS:.c=.bin) $(SHIM_SRCS:.c=.bin)
PROCS = $(SHIM_SRCS:.c=_proc)

# Default target: build all ELF binaries for KVM
all: $(RAW_ELFS) $(SHIM_ELFS)

# Target for native process execution
proc: $(PROCS)

# Target to build all Docker images
docker-all:
	@for name in $(SHIM_SRCS:.c=); do \
		echo "Building Docker image for $$name..."; \
		docker build --build-arg SOURCE=$${name}.c -t iai_$$name . ; \
	done

# Step 3: Strip the ELF headers to create the raw flat binaries
bins: $(BINS)

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
	@echo "--> Successfully generated flat binary: $@"

# Step 2A: Link RAW binaries (No Shim)
$(RAW_ELFS): %.elf: %.o
	$(LD) $(LDFLAGS_RAW) $< -o $@

# Step 2B: Link SHIM binaries (With libshim.a and start.o)
$(SHIM_ELFS): %.elf: %.o $(SHIM_START) $(SHIM_LIB)
	$(LD) $(LDFLAGS_SHIM) $(SHIM_START) $< $(SHIM_LIB) -o $@

# Step 1: Compile the C source code into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Native process build (uses standard libc)
%_proc: %.c timing.c
	$(CC) -O2 $< timing.c -o $@

# Clean up all build artifacts
clean:
	rm -f *.o *.elf *.bin *_proc
	@echo "Cleaned sample binaries."

.PHONY: all bins clean proc docker-all
