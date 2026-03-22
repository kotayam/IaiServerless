#include <string.h>
#include <unistd.h>

int main() {
  const char *msg = "Hello World!";
  write(1, msg, strlen(msg));
  return 0;
}
