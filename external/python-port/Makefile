# Makefile for IaiServerless MicroPython port

# Paths
MICROPYTHON = ../micropython
SHIM_DIR = ../../shim
BUILD = build

include $(MICROPYTHON)/py/mkenv.mk

# qstr definitions
QSTR_DEFS = qstrdefsport.h

# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 0

# include py core make definitions
include $(TOP)/py/py.mk
include $(TOP)/extmod/extmod.mk

# Compiler settings for freestanding build
CC = gcc
LD = ld
CFLAGS = -ffreestanding -nostdlib -m64 -O2 -Wall -Wextra
CFLAGS += -I$(SHIM_DIR) -I. -I$(TOP) -I$(BUILD)

# Linker settings - use shim's linker script
LDFLAGS = -T $(SHIM_DIR)/link.ld

# Shim library
SHIM_START = $(SHIM_DIR)/start.o
SHIM_LIB = $(SHIM_DIR)/libshim.a

# Source files
SRC_C = main.c

OBJ = $(PY_CORE_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o))

# Output
PROG = micropython.elf

all: $(PROG)

$(PROG): $(OBJ)
	$(LD) $(LDFLAGS) $(SHIM_START) $(OBJ) $(SHIM_LIB) -o $@
	@echo "--> Built MicroPython for IaiServerless: $@"

# Include MicroPython's build rules
include $(TOP)/py/mkrules.mk

clean:
	rm -rf $(BUILD) $(PROG)

.PHONY: all clean
