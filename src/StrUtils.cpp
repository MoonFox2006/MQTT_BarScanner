#include <stdlib.h>
#include "StrUtils.h"

bool allocStr(char **str, const char *src) {
  if (src && *src) {
    if (*str) {
      void *ptr = realloc(*str, strlen(src) + 1);

      if (! ptr)
        return false;
      *str = (char*)ptr;
    } else {
      *str = (char*)malloc(strlen(src) + 1);
      if (! *str)
        return false;
    }
    strcpy(*str, src);
  } else {
    if (*str) {
      free(*str);
      *str = NULL;
    }
  }

  return true;
}

bool allocStr_P(char **str, PGM_P src) {
  if (src && pgm_read_byte(src)) {
    if (*str) {
      void *ptr = realloc(*str, strlen_P(src) + 1);

      if (! ptr)
        return false;
      *str = (char*)ptr;
    } else {
      *str = (char*)malloc(strlen_P(src) + 1);
      if (! *str)
        return false;
    }
    strcpy_P(*str, src);
  } else {
    if (*str) {
      free(*str);
      *str = NULL;
    }
  }

  return true;
}

void disposeStr(char **str) {
  if (*str) {
    free(*str);
    *str = NULL;
  }
}

char *byteToHex(char *out, uint8_t value) {
  uint8_t b;

  b = value >> 4;
  if (b < 10)
    out[0] = '0' + b;
  else
    out[0] = 'A' + (b - 10);
  b = value & 0x0F;
  if (b < 10)
    out[1] = '0' + b;
  else
    out[1] = 'A' + (b - 10);
  out[2] = '\0';

  return out;
}
