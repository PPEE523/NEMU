#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  char buf[1024];
  va_list ap;
  va_start(ap, fmt);
  int ret = vsprintf(buf, fmt, ap);
  va_end(ap);

  for (int i = 0; buf[i]; i++) {
    putch(buf[i]);
  }

  return ret;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  char *str = out;

  while (*fmt) {
    if (*fmt != '%') {
      *str++ = *fmt++;
      continue;
    }

    fmt++;  // skip %

    if (*fmt == 'd') {
      int val = va_arg(ap, int);
      char buf[32];
      int i = 0, neg = 0;

      if (val < 0) {
        neg = 1;
        val = -val;
      }

      do {
        buf[i++] = '0' + val % 10;
        val /= 10;
      } while (val);

      if (neg) buf[i++] = '-';

      while (i--) {
        *str++ = buf[i];
      }
    }

    else if (*fmt == 'x') {
      unsigned int val = va_arg(ap, unsigned int);
      char buf[32];
      int i = 0;

      do {
        int digit = val % 16;
        buf[i++] = digit < 10 ? '0' + digit : 'a' + digit - 10;
        val /= 16;
      } while (val);

      while (i--) {
        *str++ = buf[i];
      }
    }

    else if (*fmt == 's') {
      char *s = va_arg(ap, char *);
      while (*s) {
        *str++ = *s++;
      }
    }

    else if (*fmt == 'c') {
      char c = (char)va_arg(ap, int);
      *str++ = c;
    }

    else if (*fmt == '%') {
      *str++ = '%';
    }

    fmt++;
  }

  *str = '\0';
  return str - out;
}


int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vsprintf(out, fmt, ap);
  va_end(ap);
  return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vsprintf(out, fmt, ap);
  va_end(ap);
  return ret;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  return vsprintf(out, fmt, ap);
}

#endif
