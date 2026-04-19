def handler():
    NUM_BLOCKS = 64
    SIZES = [16, 32, 64, 128, 256]

    blocks = []
    total_bytes = 0

    # Allocate and fill
    for i in range(NUM_BLOCKS):
        sz = SIZES[i % len(SIZES)]
        b = [i & 0xFF] * sz
        blocks.append(b)
        total_bytes += sz

    # Verify
    errors = 0
    for i in range(NUM_BLOCKS):
        sz = SIZES[i % len(SIZES)]
        b = blocks[i]
        for j in range(sz):
            if b[j] != (i & 0xFF):
                errors += 1
                break

    del blocks

    print("alloc_stress: %d blocks, %d bytes, %d errors" % (NUM_BLOCKS, total_bytes, errors))
