#include "unistd.h"
#include <stdarg.h>
#include <stddef.h>

static void format_num(char **buf, size_t *remaining, long long num, int base,
                       int width, int zero_pad) {
  char tmp[32];
  int i = 0, neg = 0;

  if (num < 0 && base == 10) {
    neg = 1;
    num = -num;
  }

  if (num == 0)
    tmp[i++] = '0';
  else {
    while (num && i < 31) { // Prevent buffer overflow in tmp
      int digit = num % base;
      tmp[i++] = digit < 10 ? '0' + digit : 'a' + digit - 10;
      num /= base;
    }
  }

  if (neg && i < 31)
    tmp[i++] = '-';

  // Apply width padding (capped to prevent overflow)
  while (i < width && i < 31) {
    tmp[i++] = zero_pad ? '0' : ' ';
  }

  // Copy to output buffer with bounds checking
  if (buf && *buf && *remaining > 0) {
    while (i > 0 && *remaining > 1) {
      **buf = tmp[--i];
      (*buf)++;
      (*remaining)--;
    }
  }
}

static void format_str(char **buf, size_t *remaining, const char *s) {
  if (!s)
    s = "(null)";

  // Bounds-checked string copy
  if (buf && *buf && *remaining > 0) {
    while (*s && *remaining > 1) {
      **buf = *s++;
      (*buf)++;
      (*remaining)--;
    }
  }
}

int vsnprintf(char *str, size_t size, const char *fmt, va_list ap) {
  if (!str || size == 0)
    return 0;

  char *buf = str;
  size_t remaining = size;

  while (*fmt && remaining > 1) {
    if (*fmt == '%') {
      fmt++;
      int width = 0, zero_pad = 0;

      if (*fmt == '0') {
        zero_pad = 1;
        fmt++;
      }

      // Parse width with overflow protection
      while (*fmt >= '0' && *fmt <= '9') {
        int digit = *fmt - '0';
        if (width > (0x7FFFFFFF - digit) / 10) // Prevent integer overflow
          break;
        width = width * 10 + digit;
        fmt++;
      }

      switch (*fmt) {
      case 'd':
      case 'i':
        format_num(&buf, &remaining, va_arg(ap, int), 10, width, zero_pad);
        break;
      case 'u':
        format_num(&buf, &remaining, va_arg(ap, unsigned int), 10, width,
                   zero_pad);
        break;
      case 'x':
        format_num(&buf, &remaining, va_arg(ap, unsigned int), 16, width,
                   zero_pad);
        break;
      case 'p':
        if (remaining > 3) {
          format_str(&buf, &remaining, "0x");
        }
        format_num(&buf, &remaining, (unsigned long)va_arg(ap, void *), 16,
                   width, zero_pad);
        break;
      case 's':
        format_str(&buf, &remaining, va_arg(ap, char *));
        break;
      case 'c':
        if (remaining > 1) {
          *buf++ = (char)va_arg(ap, int);
          remaining--;
        }
        break;
      case '%':
        if (remaining > 1) {
          *buf++ = '%';
          remaining--;
        }
        break;
      default:
        if (remaining > 1) {
          *buf++ = *fmt;
          remaining--;
        }
        break;
      }
      fmt++;
    } else {
      *buf++ = *fmt++;
      remaining--;
    }
  }

  // Always null-terminate
  if (size > 0)
    *buf = '\0';

  return buf - str;
}

int snprintf(char *str, size_t size, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vsnprintf(str, size, fmt, ap);
  va_end(ap);
  return ret;
}

int printf(const char *fmt, ...) {
  char buf[512]; // Increased buffer size for safety
  va_list ap;
  va_start(ap, fmt);
  int len = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  // Clamp length to buffer size to prevent out-of-bounds read
  if (len >= (int)sizeof(buf))
    len = sizeof(buf) - 1;

  write(1, buf, len);
  return len;
}

int putchar(int c) {
  char ch = (char)c;
  write(1, &ch, 1);
  return c;
}
