# IaiServerless Root Makefile

# .PHONY tells Make that these aren't physical files, but commands
.PHONY: all host shim clean

# The default target if you just type 'make' at the root
all: host

# Target to build just the host loader
host:
	@echo "Building Host Loader..."
	$(MAKE) -C host

# Target to build just the shim library (placeholder for now)
shim:
	@echo "Building Shim Library..."
	# $(MAKE) -C shim

# Clean up build artifacts in all subdirectories
clean:
	@echo "Cleaning Host..."
	$(MAKE) -C host clean
	@echo "Cleaning Shim..."
	# $(MAKE) -C shim clean
	@echo "All clean!"
