# --- Stage 1: Build the C binary ---
FROM alpine:latest AS builder

# Install the GCC compiler and standard C library
RUN apk add --no-cache gcc musl-dev

# Set up our working directory
WORKDIR /app

# Copy your C source code into the builder container
COPY prime.c .

# Compile the code statically. 
# "-static" is crucial here! It bundles the C standard library directly 
# into the binary so it doesn't need external shared libraries (like libc.so) to run.
RUN gcc -O2 -static prime.c -o prime_proc


# --- Stage 2: Create the minimal execution image ---
# 'scratch' is a special, completely empty Docker image (0 bytes).
FROM scratch

# Copy ONLY the compiled binary from the builder stage
COPY --from=builder /app/prime_proc /prime_proc

# Tell Docker to execute this binary exactly when the container starts
ENTRYPOINT ["/prime_proc"]
