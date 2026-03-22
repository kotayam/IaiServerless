# IaiServerless Root Makefile

.PHONY: all host samples shim clean

# Default target builds the host loader and the sample binaries
all: host shim samples

host:
	@echo "=== Building Host Loader ==="
	$(MAKE) -C host

shim:
	@echo "=== Building Shim Library ==="
	$(MAKE) -C shim

samples: shim
	@echo "=== Building Sample Binaries ==="
	$(MAKE) -C samples


clean:
	@echo "=== Cleaning Host ==="
	$(MAKE) -C host clean
	@echo "=== Cleaning Shim ==="
	$(MAKE) -C shim clean
	@echo "=== Cleaning Samples ==="
	$(MAKE) -C samples clean
	@echo "All clean!"
