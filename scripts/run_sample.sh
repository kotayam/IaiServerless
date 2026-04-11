#!/bin/bash

# Simple script to run a sample binary in the IaiServerless host_loader

# Check if at least one argument is provided
if [ $# -lt 1 ]; then
    echo "Usage: $0 [-v] <sample_binary.elf>"
    exit 1
fi

VERBOSE_FLAG=""
BINARY=""

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -v)
            VERBOSE_FLAG="-v"
            shift
            ;;
        *)
            BINARY=$1
            shift
            ;;
    esac
done

if [ ! -f "samples/$BINARY" ]; then
    # try relative path
    if [ ! -f "$BINARY" ]; then
        echo "Error: Binary '$BINARY' not found."
        exit 1
    fi
else
    BINARY="samples/$BINARY"
fi

echo "--- Executing $BINARY in IaiServerless ---"
./host/host_loader $VERBOSE_FLAG "$BINARY"
