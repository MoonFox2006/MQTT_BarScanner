#ifndef __HTMLHELPER_H
#define __HTMLHELPER_H

#include <WString.h>

const char TEXT_HTML[] PROGMEM = "text/html";
const char TEXT_PLAIN[] PROGMEM = "text/plain";
const char TEXT_CSS[] PROGMEM = "text/css";
const char APPLICATION_JSON[] PROGMEM = "application/json";

const char HTML_START[] PROGMEM = "<!DOCTYPE html>\n"
  "<html>\n"
  "<head>\n";
const char HEAD_END[] PROGMEM = "</head>\n"
  "<body>\n";
const char HTML_END[] PROGMEM = "</body>\n"
  "</html>";

String tag(const String &tagName, const String &tagValue, bool nl = false);
String tag(const String &tagName, bool nl = false);

String tag_P(PGM_P tagName, const String &tagValue, bool nl = false);
String tag_P(PGM_P tagName, bool nl = false);

#endif
