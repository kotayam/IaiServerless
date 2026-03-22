// Send a single byte to a hardware port
static inline void outb(unsigned short port, unsigned char value) {
  asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

// _start is the standard entry point for raw binaries
void _start() {
  const char *msg = "Hello from bare metal KVM!\n";

  // Loop through the string and send each character to port 0xE9
  for (int i = 0; msg[i] != '\0'; i++) {
    outb(0xE9, msg[i]);
  }

  // Halt the CPU to trigger KVM_EXIT_HLT (exit reason 5)
  asm volatile("hlt");
}
