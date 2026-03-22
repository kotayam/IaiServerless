# Makefile for IaiServerless Shim Library

CC = gcc
AR = ar
CFLAGS = -ffreestanding -nostdlib -m64 -O2 -Wall -Wextra

SRCS_C = shim.c
SRCS_S = start.S
OBJS = start.o shim.o

all: libshim.a

# Bundle the object files into a static archive
libshim.a: $(OBJS)
	$(AR) rcs $@ $^
	@echo "--> Built Shim Library ($@)"

# Compile C code
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile Assembly code
%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) libshim.a
