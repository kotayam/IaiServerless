// samples/test_wx_write.c

// A dummy function that lives in the .text segment
void target_function() { __asm__ volatile("nop"); }

int main() {
  // 1. Get the physical address of the function in the .text segment
  unsigned char *code_ptr = (unsigned char *)target_function;

  // 2. Attempt to overwrite the first instruction with a NOP (0x90)
  // If W^X is working, the CPU will throw a Page Fault here because
  // the .text page table entry does not have the PDE64_RW bit set!
  *code_ptr = 0x90;

  // If the program reaches this point, the security failed.
  return 1;
}
