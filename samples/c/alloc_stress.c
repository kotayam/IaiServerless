#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define NUM_BLOCKS 64

/* Sizes cycle through 16, 32, 64, 128, 256 bytes */
static const int SIZES[] = {16, 32, 64, 128, 256};
#define NUM_SIZES 5

int main() {
    void *blocks[NUM_BLOCKS];
    int total_bytes = 0;

    /* Allocate */
    for (int i = 0; i < NUM_BLOCKS; i++) {
        int sz = SIZES[i % NUM_SIZES];
        blocks[i] = malloc(sz);
        if (!blocks[i]) {
            const char *err = "Error: malloc failed\n";
            write(1, err, 20);
            return 1;
        }
        memset(blocks[i], (unsigned char)(i & 0xFF), sz);
        total_bytes += sz;
    }

    /* Verify */
    int errors = 0;
    for (int i = 0; i < NUM_BLOCKS; i++) {
        int sz = SIZES[i % NUM_SIZES];
        unsigned char *p = blocks[i];
        for (int j = 0; j < sz; j++) {
            if (p[j] != (unsigned char)(i & 0xFF)) {
                errors++;
                break;
            }
        }
    }

    /* Free */
    for (int i = 0; i < NUM_BLOCKS; i++)
        free(blocks[i]);

    write(1, "HTTP/1.1 200 OK\r\n\r\n", 19);

    char buf[128];
    int len = snprintf(buf, sizeof(buf),
        "alloc_stress: %d blocks, %d bytes, %d errors\n",
        NUM_BLOCKS, total_bytes, errors);
    write(1, buf, len);

    return 0;
}
