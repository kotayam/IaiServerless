// samples/test_wx_exec.c

int main() {
  // 1. Put valid x86-64 machine code on the Stack (which is Read/Write)
  // 0xC3 is the hexadecimal machine code for the 'ret' instruction.
  unsigned char shellcode[] = {0xC3};

  // 2. Trick the C compiler into treating the array as a function
  void (*malicious_func)() = (void (*)())shellcode;

  // 3. Attempt to jump the Instruction Pointer (RIP) to the Stack!
  // If W^X is working, the CPU will throw a Page Fault here because
  // the Stack page table entry has the PTE64_NX (No-Execute) bit set!
  malicious_func();

  // If the program reaches this point, the security failed.
  return 1;
}
