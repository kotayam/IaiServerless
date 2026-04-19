#!/bin/bash
# Build a Python serverless function by embedding the .py file into MicroPython runtime

set -e

if [ $# -ne 1 ]; then
    echo "Usage: $0 <python_file.py>"
    exit 1
fi

PYTHON_FILE="$1"
BASENAME=$(basename "$PYTHON_FILE" .py)
OUTPUT="${BASENAME}.elf"
PYTHON_PORT_DIR="../../external/python-port"

echo "=== Building Python function: $OUTPUT ==="

# Convert Python source to object file
objcopy -I binary -O elf64-x86-64 -B i386:x86-64 \
    --rename-section .data=.rodata,alloc,load,readonly,data,contents \
    --redefine-sym "_binary_${BASENAME}_py_start=_binary_code_py_start" \
    --redefine-sym "_binary_${BASENAME}_py_end=_binary_code_py_end" \
    "$PYTHON_FILE" "${BASENAME}.py.o"

# Build MicroPython with embedded Python code
make -C "$PYTHON_PORT_DIR" EXTRA_OBJ="$(pwd)/${BASENAME}.py.o" micropython.elf

# Copy result
cp "$PYTHON_PORT_DIR/micropython.elf" "$OUTPUT"

# Show size
size "$OUTPUT"

# Cleanup
rm -f "${BASENAME}.py.o"

echo "==> Built: $OUTPUT"
