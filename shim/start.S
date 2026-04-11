.global _start
.extern main

.section .text.entry
_start:
    # Mark the end of the stack frame chain
    xor %rbp, %rbp
    # Align the stack pointer to a 16-byte boundary
    and $-16, %rsp

    # Call the user's main function
    call main

    # If main() returns, halt the CPU to trigger KVM_EXIT_HLT
.halt_loop:
    hlt
    jmp .halt_loop
