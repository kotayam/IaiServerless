#include <unistd.h>
#include <stdio.h>

int main() {
  /* Read N from stdin, default 10000 */
  char buf[16]; int blen = 0; long n;
  while (blen < 15 && (n = read(0, buf + blen, 15 - blen)) > 0) blen += n;
  int target = 0;
  for (int i = 0; i < blen; i++) {
    if (buf[i] >= '0' && buf[i] <= '9') target = target * 10 + (buf[i] - '0');
    else break;
  }
  if (target <= 0) target = 10000;
  int count = 0;
  int num = 2;
  int last_prime = 2;

  // CPU bound calculation
  while (count < target) {
    int is_prime = 1;
    for (int i = 2; i * i <= num; i++) {
      if (num % i == 0) {
        is_prime = 0;
        break;
      }
    }
    if (is_prime) {
      count++;
      last_prime = num;
    }
    num++;
  }

  // format output
  char out_buf[128];
  int len = snprintf(out_buf, sizeof(out_buf),
      "Compute Task Complete. %dth prime is: %d\n", target, last_prime);
  write(1, out_buf, len);
  return 0;
}
