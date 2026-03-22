#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Initialize variables
MEM_FLAG=""

# Parse optional arguments (like -m)
while getopts "m:" opt; do
    case $opt in
    m)
        MEM_FLAG="-m $OPTARG"
        ;;
    \?)
        echo "Usage: $0 [-m memory_in_MB] <binary_name.bin>"
        exit 1
        ;;
    esac
done

# Shift the parsed options away so $1 becomes the binary name
shift $((OPTIND - 1))

# 1. Check if the user provided the binary argument
if [ -z "$1" ]; then
    echo "Usage: $0 [-m memory_in_MB] <binary_name.bin>"
    echo "Example: $0 -m 4 bare_hello.bin"
    exit 1
fi

BIN_NAME=$1
HOST_LOADER="./host/host_loader"
SAMPLE_PATH="./samples/${BIN_NAME}"

# 2. Verify the host loader exists
if [ ! -f "$HOST_LOADER" ]; then
    echo "Error: Host loader not found at $HOST_LOADER."
    echo "Please run 'make host' from the root directory first."
    exit 1
fi

# 3. Verify the sample binary exists
if [ ! -f "$SAMPLE_PATH" ]; then
    echo "Error: Sample binary not found at $SAMPLE_PATH."
    echo "Please run 'make samples' first."
    exit 1
fi

# 4. Execute the VM
echo "--- Executing $BIN_NAME in IaiServerless ---"

# Pass the memory flag if it was provided, otherwise just run normally
$HOST_LOADER $MEM_FLAG "$SAMPLE_PATH"
