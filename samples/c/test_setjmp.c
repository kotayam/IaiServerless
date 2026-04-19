#include <setjmp.h>
#include <stdio.h>

static jmp_buf exception_buf;
static int exception_code = 0;

void throw_exception(int code) {
  exception_code = code;
  longjmp(exception_buf, code);
}

int risky_function(int should_fail) {
  printf("  Entering risky_function(%d)\n", should_fail);
  if (should_fail) {
    printf("  Throwing exception!\n");
    throw_exception(42);
  }
  printf("  Exiting normally\n");
  return 100;
}

int main() {
  printf("Testing setjmp/longjmp...\n\n");

  // Test 1: Normal execution (no exception)
  printf("=== Test 1: Normal execution ===\n");
  int ret = setjmp(exception_buf);
  if (ret == 0) {
    printf("setjmp returned 0 (first call)\n");
    int result = risky_function(0);
    printf("Result: %d\n", result);
  } else {
    printf("ERROR: Should not reach here!\n");
  }

  // Test 2: Exception handling
  printf("\n=== Test 2: Exception handling ===\n");
  ret = setjmp(exception_buf);
  if (ret == 0) {
    printf("setjmp returned 0 (first call)\n");
    risky_function(1);
    printf("ERROR: Should not reach here!\n");
  } else {
    printf("setjmp returned %d (exception caught)\n", ret);
    printf("Exception code: %d\n", exception_code);
  }

  // Test 3: Nested calls
  printf("\n=== Test 3: Nested exception ===\n");
  ret = setjmp(exception_buf);
  if (ret == 0) {
    printf("Calling nested functions...\n");
    risky_function(0);
    risky_function(1);
    printf("ERROR: Should not reach here!\n");
  } else {
    printf("Caught exception: %d\n", ret);
  }

  // Test 4: Multiple setjmp/longjmp
  printf("\n=== Test 4: Multiple jumps ===\n");
  for (int i = 1; i <= 3; i++) {
    ret = setjmp(exception_buf);
    if (ret == 0) {
      printf("Iteration %d: throwing %d\n", i, i * 10);
      throw_exception(i * 10);
    } else {
      printf("Iteration %d: caught %d\n", i, ret);
    }
  }

  printf("\nAll tests passed!\n");
  return 0;
}
