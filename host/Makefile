# Makefile for IaiServerless Host Loader

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Werror -Wextra -O2 -g
LDFLAGS = 

# The name of your final executable
TARGET = host_loader

# Automatically find all .c files in the host/ directory
SRCS = $(wildcard *.c)

# Replace the .c extension with .o for the object files
OBJS = $(SRCS:.c=.o)

# The default target that runs when you just type 'make'
all: $(TARGET)

# Rule to link the object files into the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Rule to compile each .c file into a .o object file
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to clean up build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets aren't real files
.PHONY: all clean
