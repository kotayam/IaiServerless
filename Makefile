# IaiServerless Root Makefile

.PHONY: all host shim samples gateway python clean

# Default target builds the host loader and the sample binaries
all: host shim samples gateway

host:
	@echo "=== Building Host Loader ==="
	$(MAKE) -C host

shim:
	@echo "=== Building Shim Library ==="
	$(MAKE) -C shim

samples: shim
	@echo "=== Building Sample Binaries ==="
	$(MAKE) -C samples

gateway:
	@echo "=== Building Gateway ==="
	cd gateway && go build -o gateway main.go

python: shim
	@echo "=== Building MicroPython Runtime ==="
	$(MAKE) -C external/python-port

clean:
	@echo "=== Cleaning Host ==="
	$(MAKE) -C host clean
	@echo "=== Cleaning Shim ==="
	$(MAKE) -C shim clean
	@echo "=== Cleaning Samples ==="
	$(MAKE) -C samples clean
	@echo "=== Cleaning Gateway ==="
	rm -f gateway/iai-gateway
	@echo "=== Cleaning MicroPython ==="
	$(MAKE) -C external/python-port clean
	@echo "All clean!"
