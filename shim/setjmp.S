.section .text

# int setjmp(jmp_buf env)
# Save callee-saved registers and return address
.globl setjmp
.globl _setjmp
.type setjmp, @function
.type _setjmp, @function
setjmp:
_setjmp:
    # env is in %rdi (first argument)
    # Save callee-saved registers
    movq %rbx, 0(%rdi)      # rbx
    movq %rbp, 8(%rdi)      # rbp
    movq %r12, 16(%rdi)     # r12
    movq %r13, 24(%rdi)     # r13
    movq %r14, 32(%rdi)     # r14
    movq %r15, 40(%rdi)     # r15
    
    # Save stack pointer (before call)
    leaq 8(%rsp), %rax
    movq %rax, 48(%rdi)     # rsp
    
    # Save return address
    movq (%rsp), %rax
    movq %rax, 56(%rdi)     # rip
    
    # Return 0
    xorl %eax, %eax
    ret

# void longjmp(jmp_buf env, int val)
# Restore saved registers and jump back
.globl longjmp
.type longjmp, @function
longjmp:
    # env is in %rdi, val is in %esi
    # Restore callee-saved registers
    movq 0(%rdi), %rbx      # rbx
    movq 8(%rdi), %rbp      # rbp
    movq 16(%rdi), %r12     # r12
    movq 24(%rdi), %r13     # r13
    movq 32(%rdi), %r14     # r14
    movq 40(%rdi), %r15     # r15
    
    # Restore stack pointer
    movq 48(%rdi), %rsp     # rsp
    
    # Set return value (val, or 1 if val is 0)
    movl %esi, %eax
    testl %eax, %eax
    jnz 1f
    incl %eax               # if val == 0, return 1
1:
    # Jump to saved return address
    jmpq *56(%rdi)          # rip
