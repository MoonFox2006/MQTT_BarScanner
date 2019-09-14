#ifdef ESP32
#include <SPIFFS.h>
#else
#include <FS.h>
#endif
#include "BaseConfig.h"

bool BaseConfig::load() {
  char mode[2];

  mode[0] = 'r';
  mode[1] = '\0';

  File file = SPIFFS.open(FPSTR(CONFIG_FILE_NAME), mode);

  if (file) {
    DynamicJsonDocument jsonDoc(JSON_BUF_SIZE);
    DeserializationError error = deserializeJson(jsonDoc, file);

    file.close();
    if (! error) {
      read(jsonDoc);

      return true;
    }
  }

  return false;
}

bool BaseConfig::save() {
  char mode[2];

  mode[0] = 'w';
  mode[1] = '\0';

  File file = SPIFFS.open(FPSTR(CONFIG_FILE_NAME), mode);

  if (file) {
    DynamicJsonDocument jsonDoc(JSON_BUF_SIZE);

    write(jsonDoc);
    serializeJson(jsonDoc, file);
    file.close();

    return true;
  }

  return false;
}

String BaseConfig::toString() {
  String result;
  DynamicJsonDocument jsonDoc(JSON_BUF_SIZE);

  write(jsonDoc);
  serializeJsonPretty(jsonDoc, result);

  return result;
}

bool BaseConfig::fromString(const String &str) {
  DynamicJsonDocument jsonDoc(JSON_BUF_SIZE);
  DeserializationError error = deserializeJson(jsonDoc, str);

  if (! error) {
    read(jsonDoc);

    return true;
  }

  return false;
}

bool initSPIFFS() {
#ifdef ESP32
  return SPIFFS.begin(true);
#else
  if (! SPIFFS.begin()) {
    if ((! SPIFFS.format()) || (! SPIFFS.begin())) {
      return false;
    }
  }
  return true;
#endif
}
