#ifndef __STRUTILS_H
#define __STRUTILS_H

#include <inttypes.h>
#include <pgmspace.h>
#include <WString.h>

bool allocStr(char **str, const char *src);
bool allocStr_P(char **str, PGM_P src);
inline bool allocStr(char **str, const __FlashStringHelper *src);
void disposeStr(char **str);

char *byteToHex(char *out, uint8_t value);

#endif
