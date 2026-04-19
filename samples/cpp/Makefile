# Makefile for C++ samples

CXX = g++
LD = ld

CXXFLAGS = -ffreestanding -m64 -O2 -Wall -Wextra -fno-exceptions -fno-rtti
LDFLAGS = -T $(SHIM_DIR)/link.ld
LDLIBS = -lstdc++

SHIM_DIR = ../../shim
SHIM_START = $(SHIM_DIR)/start.o
SHIM_LIB = $(SHIM_DIR)/libshim.a

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
ELFS = $(SRCS:.cpp=.elf)

all: $(ELFS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.elf: %.o $(SHIM_START) $(SHIM_LIB)
	$(LD) $(LDFLAGS) $(SHIM_START) $< $(SHIM_LIB) $(LDLIBS) -o $@

docker-all:
	@echo "Building Docker images for C++ samples..."
	@for src in $(SRCS); do \
		name=$${src%.cpp}; \
		echo "Building Docker image for $$name..."; \
		docker build --build-arg SOURCE=$$src -t iai_cpp_$$name . ; \
	done

clean:
	rm -f *.o *.elf

.PHONY: all docker-all clean
