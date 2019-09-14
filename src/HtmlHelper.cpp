#include "HtmlHelper.h"

String tag(const String &tagName, const String &tagValue, bool nl) {
  String result;

  result += '<';
  result += tagName;
  result += '>';
  result += tagValue;
  result += PSTR("</");
  result += tagName;
  result += '>';
  if (nl)
    result += '\n';

  return result;
}

String tag(const String &tagName, bool nl) {
  String result;

  result += '<';
  result += tagName;
  result += FPSTR("/>");
  if (nl)
    result += '\n';

  return result;
}

String tag_P(PGM_P tagName, const String &tagValue, bool nl) {
  String result;

  result += '<';
  result += FPSTR(tagName);
  result += '>';
  result += tagValue;
  result += PSTR("</");
  result += FPSTR(tagName);
  result += '>';
  if (nl)
    result += '\n';

  return result;
}

String tag_P(PGM_P tagName, bool nl) {
  String result;

  result += '<';
  result += FPSTR(tagName);
  result += FPSTR("/>");
  if (nl)
    result += '\n';

  return result;
}
