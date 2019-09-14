#ifndef __BASECONFIG_H
#define __BASECONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>

const char CONFIG_FILE_NAME[] PROGMEM = "/config.json";

class BaseConfig {
public:
  virtual bool load();
  virtual bool save();
  virtual void clear() = 0;

  virtual String toString();
  virtual bool fromString(const String &str);

protected:
  static const uint16_t JSON_BUF_SIZE = 1024;

  virtual void read(const JsonDocument &doc) = 0;
  virtual void write(JsonDocument &doc) = 0;
};

bool initSPIFFS();

#endif
