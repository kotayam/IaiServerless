#ifndef SHIM_SETJMP_H
#define SHIM_SETJMP_H

// jmp_buf stores: rbx, rbp, r12-r15, rsp, rip (8 registers)
typedef unsigned long jmp_buf[8];

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val) __attribute__((noreturn));

#endif /* SHIM_SETJMP_H */
