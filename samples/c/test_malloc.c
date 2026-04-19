long write(int fd, const void *buf, unsigned long count);
void *malloc(unsigned long size);
void free(void *ptr);

void print_str(const char *s) {
    unsigned long len = 0;
    while (s[len]) len++;
    write(1, s, len);
}

void print_hex(unsigned long val) {
    char buf[19] = "0x";
    char *p = buf + 2;
    for (int i = 60; i >= 0; i -= 4) {
        int digit = (val >> i) & 0xF;
        *p++ = digit < 10 ? '0' + digit : 'a' + digit - 10;
    }
    *p++ = '\n';
    write(1, buf, p - buf);
}

int main() {
    print_str("Testing malloc/free...\n");
    
    // Test 1: Basic allocation
    void *p1 = malloc(64);
    print_str("malloc(64): ");
    print_hex((unsigned long)p1);
    
    // Test 2: Multiple allocations
    void *p2 = malloc(128);
    print_str("malloc(128): ");
    print_hex((unsigned long)p2);
    
    void *p3 = malloc(256);
    print_str("malloc(256): ");
    print_hex((unsigned long)p3);
    
    // Test 3: Free and reuse
    free(p2);
    print_str("freed p2\n");
    
    void *p4 = malloc(100);  // Should reuse p2's block
    print_str("malloc(100): ");
    print_hex((unsigned long)p4);
    
    // Test 4: Free NULL (should not crash)
    free((void *)0);
    print_str("free(NULL) - OK\n");
    
    // Test 5: Double free (should be ignored)
    free(p1);
    free(p1);
    print_str("double free - OK\n");
    
    // Test 6: Free invalid pointer (should be ignored)
    free((void *)0x1000);
    print_str("free(invalid) - OK\n");
    
    // Test 7: Free misaligned pointer (should be ignored)
    free((char *)p3 + 4);
    print_str("free(misaligned) - OK\n");
    
    // Test 8: Large allocation
    void *p5 = malloc(1024 * 1024);  // 1MB
    print_str("malloc(1MB): ");
    print_hex((unsigned long)p5);
    
    // Test 9: Write to allocated memory
    if (p5) {
        char *buf = (char *)p5;
        for (int i = 0; i < 10; i++) {
            buf[i] = 'A' + i;
        }
        buf[10] = '\0';
        print_str("Written to p5: ");
        print_str(buf);
        print_str("\n");
    }
    
    print_str("All tests passed!\n");
    return 0;
}
