# Makefile for IaiServerless Shim Library

CC = gcc
AR = ar
CFLAGS = -ffreestanding -nostdlib -m64 -O2 -Wall -Wextra -U_FORTIFY_SOURCE

SRCS_C = hypercall.c unistd.c netdb.c string.c stdlib.c stdio.c
SRCS_S = start.S setjmp.S
OBJS = start.o hypercall.o unistd.o netdb.o string.o stdlib.o stdio.o setjmp.o

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
