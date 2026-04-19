/*
 * timing.c - Measures execution time using CLOCK_MONOTONIC entirely within
 * the process. Emits "X-Exec-Time: <ms>" to stderr on exit so the gateway
 * can compute cold start as: cold_start = e2e - exec_time.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static struct timespec _start;

static void emit_exec_time(void) {
  struct timespec end;
  clock_gettime(CLOCK_MONOTONIC, &end);
  double ms =
      (end.tv_sec - _start.tv_sec) * 1e3 + (end.tv_nsec - _start.tv_nsec) / 1e6;
  fprintf(stderr, "X-Exec-Time: %.4f\n", ms);
}

__attribute__((constructor)) static void timing_init(void) {
  clock_gettime(CLOCK_MONOTONIC, &_start);
  atexit(emit_exec_time);
}
