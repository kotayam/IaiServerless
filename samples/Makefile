# Makefile for IaiServerless Bare-Metal Samples

CC = gcc
LD = ld
OBJCOPY = objcopy

# Bare-metal compiler flags
CFLAGS = -ffreestanding -nostdlib -m64 -O2 -Wall -Wextra
# Linker flags: force code to start at physical address 0x0
LDFLAGS = -Ttext 0x0

# Automatically find all .c files in this directory
SRCS = $(wildcard *.c)

# Define the intermediate and final output files
OBJS = $(SRCS:.c=.o)
ELFS = $(SRCS:.c=.elf)
BINS = $(SRCS:.c=.bin)

# Default target: build all flat binaries
all: $(BINS)

# Step 3: Strip the ELF headers to create the raw flat binary
%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
	@echo "--> Successfully generated flat binary: $@"

# Step 2: Link the object file into an ELF executable at address 0x0
%.elf: %.o
	$(LD) $(LDFLAGS) $< -o $@

# Step 1: Compile the C source code into a freestanding object file
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up all build artifacts
clean:
	rm -f $(OBJS) $(ELFS) $(BINS)

.PHONY: all clean
