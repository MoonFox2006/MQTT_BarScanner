#ifndef __CAPTIVEPORTAL_H
#define __CAPTIVEPORTAL_H

#ifdef ESP32
#include <WiFi.h>
#include <WebServer.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#endif
#include <DNSServer.h>
#include "Customization.h"
#include "BaseConfig.h"
#ifdef USE_LED
#include "Leds.h"
#endif

class CaptivePortal {
public:
#ifdef USE_LED
  CaptivePortal(const BaseConfig *config, const Led *led) : _config((BaseConfig*)config), _led((Led*)led), _http(NULL), _dns(NULL) {}
#else
  CaptivePortal(const BaseConfig *config) : _config((BaseConfig*)config), _http(NULL), _dns(NULL) {}
#endif
  virtual ~CaptivePortal() {
    if (_http)
      delete[] _http;
    if (_dns)
      delete[] _dns;
  }

  virtual bool exec();

  virtual String ssid() const;
  virtual String password() const;
  virtual uint8_t channel() const;

protected:
  static const uint32_t CP_DURATION = 45000; // 45 sec.

#ifdef USE_LED
  static const ledmode_t LED_CPWAITING = LED_2HZ;
  static const ledmode_t LED_CPPROCESSING = LED_FADEINOUT;
#endif

  virtual void cleanup();
  virtual void restart();

  virtual void setupHandles();

  virtual void handleNotFound();
  virtual void handleCss();
  virtual void handleRoot();
  virtual void handleWriteConfig();
  virtual void handleRestart();
  virtual void handleSPIFFS();
  virtual void handleFileUploaded();
  virtual void handleFileUpload();
  virtual void handleFileDelete();
  virtual void handleFwUpdate();
  virtual void handleSketchUpdated();
  virtual void handleSketchUpdate();
#ifdef USE_AUTHORIZATION
  virtual bool checkAuthorization();
#endif
  virtual String getContentType(const String &fileName);
  virtual bool handleFileRead(const String &path);
  virtual String getCss();

  virtual bool isCaptivePortal();

  BaseConfig *_config;
#ifdef USE_LED
  Led *_led;
#endif
#ifdef ESP32
  WebServer *_http;
#else
  ESP8266WebServer *_http;
#endif
  DNSServer *_dns;
};

#endif
