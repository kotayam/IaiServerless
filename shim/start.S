.global _start
.extern main

.section .text
_start:
    # Note: Our host_loader (run_long_mode) already sets up a 2MB stack 
    # and sets the 'rsp' register for us. 
    
    # Call the user's main function
    call main

    # If main() returns, halt the CPU to trigger KVM_EXIT_HLT
.halt_loop:
    hlt
    jmp .halt_loop
