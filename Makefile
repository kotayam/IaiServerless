# IaiServerless Root Makefile

.PHONY: all host samples shim clean

# Default target builds the host loader and the sample binaries
all: host samples

host:
	@echo "=== Building Host Loader ==="
	$(MAKE) -C host

samples:
	@echo "=== Building Sample Binaries ==="
	$(MAKE) -C samples

shim:
	@echo "=== Building Shim Library ==="
	# $(MAKE) -C shim

clean:
	@echo "=== Cleaning Host ==="
	$(MAKE) -C host clean
	@echo "=== Cleaning Samples ==="
	$(MAKE) -C samples clean
	@echo "All clean!"
