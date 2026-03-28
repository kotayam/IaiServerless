#include <unistd.h>

void reverse(char str[], int length) {
  int start = 0, end = length - 1;
  while (start < end) {
    char temp = str[start];
    str[start] = str[end];
    str[end] = temp;
    start++;
    end--;
  }
}

int itoa(int num, char *str) {
  int i = 0;
  if (num == 0) {
    str[i++] = '0';
    str[i] = '\0';
    return i;
  }
  while (num != 0) {
    int rem = num % 10;
    str[i++] = rem + '0';
    num = num / 10;
  }
  str[i] = '\0';
  reverse(str, i);
  return i;
}

int main() {
  int target = 10000;
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
  char num_buf[32];
  char *prefix = "Compute Task Complete. 10,000th prime is: ";
  int p_len = 0;
  while (prefix[p_len] != '\0') {
    out_buf[p_len] = prefix[p_len];
    p_len++;
  }
  int n_len = itoa(last_prime, num_buf);
  for (int i = 0; i < n_len; i++) {
    out_buf[p_len + i] = num_buf[i];
  }
  out_buf[p_len + n_len] = '\n';

  // send back output
  write(1, out_buf, p_len + n_len + 1);
  return 0;
}
