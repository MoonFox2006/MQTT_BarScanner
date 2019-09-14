#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <AsyncMqttClient.h>
#include "StrUtils.h"
#include "BaseConfig.h"
#include "CaptivePortal.h"
#include "Buttons.h"
#include "Leds.h"

#define MQTT_QOS 0

const uint8_t BTN_PIN = 0;
const uint8_t LED_PIN = 2;
const bool LED_LEVEL = LOW;

const uint8_t BARCODE_SIZE = 127;
const char BARCODE_TERMINATOR = '\r';

class Config : public BaseConfig {
public:
  void clear();

  struct __packed {
    char *_wifi_ssid;
    char *_wifi_pswd;
    char *_mqtt_server;
    char *_mqtt_user;
    char *_mqtt_pswd;
    char *_mqtt_client;
    char *_mqtt_barcode_topic;
    char *_mqtt_button_topic;
    uint16_t _mqtt_port;
    bool _mqtt_retained;
  };

protected:
  void read(const JsonDocument &doc);
  void write(JsonDocument &doc);

  void genMqttClient();
};

static const char WIFI_SSID_PARAM[] PROGMEM = "wifi_ssid";
static const char WIFI_PSWD_PARAM[] PROGMEM = "wifi_pswd";
static const char MQTT_SERVER_PARAM[] PROGMEM = "mqtt_server";
static const char MQTT_PORT_PARAM[] PROGMEM = "mqtt_port";
static const char MQTT_USER_PARAM[] PROGMEM = "mqtt_user";
static const char MQTT_PSWD_PARAM[] PROGMEM = "mqtt_pswd";
static const char MQTT_CLIENT_PARAM[] PROGMEM = "mqtt_client";
static const char MQTT_RETAINED_PARAM[] PROGMEM = "mqtt_retained";
static const char MQTT_BARCODE_TOPIC_PARAM[] PROGMEM = "mqtt_barcode_topic";
static const char MQTT_BUTTON_TOPIC_PARAM[] PROGMEM = "mqtt_button_topic";

//#define DEF_WIFI_SSID "ssid"
//#define DEF_WIFI_PSWD "pswd"
//#define DEF_MQTT_SERVER "mqtt"
//#define DEF_MQTT_PORT 1883
//#define DEF_MQTT_USER "user"
//#define DEF_MQTT_PSWD "pswd"
//#define DEF_MQTT_CLIENT "ESP_BarScanner"
//#define DEF_MQTT_RETAINED true
#define DEF_MQTT_BARCODE_TOPIC "/barcode"
#define DEF_MQTT_BUTTON_TOPIC "/button"

void Config::clear() {
#ifdef DEF_WIFI_SSID
  allocStr_P(&_wifi_ssid, PSTR(DEF_WIFI_SSID));
#else
  disposeStr(&_wifi_ssid);
#endif
#ifdef DEF_WIFI_PSWD
  allocStr_P(&_wifi_pswd, PSTR(DEF_WIFI_PSWD));
#else
  disposeStr(&_wifi_pswd);
#endif
#ifdef DEF_MQTT_SERVER
  allocStr_P(&_mqtt_server, PSTR(DEF_MQTT_SERVER));
#else
  disposeStr(&_mqtt_server);
#endif
#ifdef DEF_MQTT_PORT
  _mqtt_port = DEF_MQTT_PORT;
#else
  _mqtt_port = 1883;
#endif
#ifdef DEF_MQTT_USER
  allocStr_P(&_mqtt_user, PSTR(DEF_MQTT_USER));
#else
  disposeStr(&_mqtt_user);
#endif
#ifdef DEF_MQTT_PSWD
  allocStr_P(&_mqtt_pswd, PSTR(DEF_MQTT_PSWD));
#else
  disposeStr(&_mqtt_pswd);
#endif
#ifdef DEF_MQTT_CLIENT
  allocStr_P(&_mqtt_client, PSTR(DEF_MQTT_CLIENT));
#else
  genMqttClient();
#endif
#ifdef DEF_MQTT_RETAINED
  _mqtt_retained = DEF_MQTT_RETAINED;
#else
  _mqtt_retained = false;
#endif
#ifdef DEF_MQTT_BARCODE_TOPIC
  allocStr_P(&_mqtt_barcode_topic, PSTR(DEF_MQTT_BARCODE_TOPIC));
#else
  disposeStr(&_mqtt_barcode_topic);
#endif
#ifdef DEF_MQTT_BUTTON_TOPIC
  allocStr_P(&_mqtt_button_topic, PSTR(DEF_MQTT_BUTTON_TOPIC));
#else
  disposeStr(&_mqtt_button_topic);
#endif
}

void Config::read(const JsonDocument &doc) {
  if (doc.containsKey(FPSTR(WIFI_SSID_PARAM)))
    allocStr(&_wifi_ssid, doc[FPSTR(WIFI_SSID_PARAM)].as<const char*>());
  else
#ifdef DEF_WIFI_SSID
    allocStr_P(&_wifi_ssid, PSTR(DEF_WIFI_SSID));
#else
    disposeStr(&_wifi_ssid);
#endif
  if (doc.containsKey(FPSTR(WIFI_PSWD_PARAM)))
    allocStr(&_wifi_pswd, doc[FPSTR(WIFI_PSWD_PARAM)].as<const char*>());
  else
#ifdef DEF_WIFI_PSWD
    allocStr_P(&_wifi_pswd, PSTR(DEF_WIFI_PSWD));
#else
    disposeStr(&_wifi_pswd);
#endif
  if (doc.containsKey(FPSTR(MQTT_SERVER_PARAM)))
    allocStr(&_mqtt_server, doc[FPSTR(MQTT_SERVER_PARAM)].as<const char*>());
  else
#ifdef DEF_MQTT_SERVER
    allocStr_P(&_mqtt_server, PSTR(DEF_MQTT_SERVER));
#else
    disposeStr(&_mqtt_server);
#endif
  if (doc.containsKey(FPSTR(MQTT_PORT_PARAM)))
    _mqtt_port = doc[FPSTR(MQTT_PORT_PARAM)];
  else
#ifdef DEF_MQTT_PORT
    _mqtt_port = DEF_MQTT_PORT;
#else
    _mqtt_port = 1883;
#endif
  if (doc.containsKey(FPSTR(MQTT_USER_PARAM)))
    allocStr(&_mqtt_user, doc[FPSTR(MQTT_USER_PARAM)].as<const char*>());
  else
#ifdef DEF_MQTT_USER
    allocStr_P(&_mqtt_user, PSTR(DEF_MQTT_USER));
#else
    disposeStr(&_mqtt_user);
#endif
  if (doc.containsKey(FPSTR(MQTT_PSWD_PARAM)))
    allocStr(&_mqtt_pswd, doc[FPSTR(MQTT_PSWD_PARAM)].as<const char*>());
  else
#ifdef DEF_MQTT_PSWD
    allocStr_P(&_mqtt_pswd, PSTR(DEF_MQTT_PSWD));
#else
    disposeStr(&_mqtt_pswd);
#endif
  if (doc.containsKey(FPSTR(MQTT_CLIENT_PARAM)))
    allocStr(&_mqtt_client, doc[FPSTR(MQTT_CLIENT_PARAM)].as<const char*>());
  else
#ifdef DEF_MQTT_CLIENT
    allocStr_P(&_mqtt_client, PSTR(DEF_MQTT_CLIENT));
#else
    genMqttClient();
#endif
  if (doc.containsKey(FPSTR(MQTT_RETAINED_PARAM)))
    _mqtt_retained = doc[FPSTR(MQTT_RETAINED_PARAM)];
  else
#ifdef DEF_MQTT_RETAINED
    _mqtt_retained = DEF_MQTT_RETAINED;
#else
    _mqtt_retained = false;
#endif
  if (doc.containsKey(FPSTR(MQTT_BARCODE_TOPIC_PARAM)))
    allocStr(&_mqtt_barcode_topic, doc[FPSTR(MQTT_BARCODE_TOPIC_PARAM)].as<const char*>());
  else
#ifdef DEF_MQTT_BARCODE_TOPIC
    allocStr_P(&_mqtt_barcode_topic, PSTR(DEF_MQTT_BARCODE_TOPIC));
#else
    disposeStr(&_mqtt_barcode_topic);
#endif
  if (doc.containsKey(FPSTR(MQTT_BUTTON_TOPIC_PARAM)))
    allocStr(&_mqtt_button_topic, doc[FPSTR(MQTT_BUTTON_TOPIC_PARAM)].as<const char*>());
  else
#ifdef DEF_MQTT_BUTTON_TOPIC
    allocStr_P(&_mqtt_button_topic, PSTR(DEF_MQTT_BUTTON_TOPIC));
#else
    disposeStr(&_mqtt_button_topic);
#endif
}

void Config::write(JsonDocument &doc) {
  char EMPTY_STR[1];

  EMPTY_STR[0] = '\0'; // ""
  doc[FPSTR(WIFI_SSID_PARAM)] = _wifi_ssid ? _wifi_ssid : EMPTY_STR;
  doc[FPSTR(WIFI_PSWD_PARAM)] = _wifi_pswd ? _wifi_pswd : EMPTY_STR;
  doc[FPSTR(MQTT_SERVER_PARAM)] = _mqtt_server ? _mqtt_server : EMPTY_STR;
  doc[FPSTR(MQTT_PORT_PARAM)] = _mqtt_port;
  doc[FPSTR(MQTT_USER_PARAM)] = _mqtt_user ? _mqtt_user : EMPTY_STR;
  doc[FPSTR(MQTT_PSWD_PARAM)] = _mqtt_pswd ? _mqtt_pswd : EMPTY_STR;
  doc[FPSTR(MQTT_CLIENT_PARAM)] = _mqtt_client ? _mqtt_client : EMPTY_STR;
  doc[FPSTR(MQTT_RETAINED_PARAM)] = _mqtt_retained;
  doc[FPSTR(MQTT_BARCODE_TOPIC_PARAM)] = _mqtt_barcode_topic ? _mqtt_barcode_topic : EMPTY_STR;
  doc[FPSTR(MQTT_BUTTON_TOPIC_PARAM)] = _mqtt_button_topic ? _mqtt_button_topic : EMPTY_STR;
}

void Config::genMqttClient() {
  static const char MQTT_CLIENT_PREFIX[] PROGMEM = "ESP_";

  uint32_t id;
  char _client[sizeof(MQTT_CLIENT_PREFIX) + 8];

  id = ESP.getChipId();
  strcpy_P(_client, MQTT_CLIENT_PREFIX);
  for (int8_t i = 0; i < 4; ++i) {
    byteToHex(&_client[i * 2 + sizeof(MQTT_CLIENT_PREFIX) - 1], id >> ((3 - i) * 8));
  }
  allocStr(&_mqtt_client, _client);
}

Config *config;
WiFiEventHandler wifiConnectHandler;
AsyncMqttClient *mqtt = NULL;
volatile uint32_t wifiLastConnecting = 0;
volatile uint32_t mqttLastConnecting = 0;
EventQueue *events;
Button *btn;
Led *led;
char barcode[BARCODE_SIZE + 1];

static void wifiConnect() {
  const uint32_t WIFI_CONNECT_TIMEOUT = 60000; // 60 sec.

  if ((! wifiLastConnecting) || (millis() - wifiLastConnecting >= WIFI_CONNECT_TIMEOUT)) {
#ifdef USE_SERIAL
    Serial.print(F("Connecting to SSID \""));
    Serial.print(config->_wifi_ssid);
    Serial.println(F("\"..."));
#endif
    WiFi.disconnect();
    WiFi.begin(config->_wifi_ssid, config->_wifi_pswd);
    wifiLastConnecting = millis();
    led->setMode(LED_FADEIN);
  }
}

static void mqttConnect() {
  const uint32_t MQTT_CONNECT_TIMEOUT = 60000; // 60 sec.

  if ((! mqttLastConnecting) || (millis() - mqttLastConnecting >= MQTT_CONNECT_TIMEOUT)) {
#ifdef USE_SERIAL
    Serial.print(F("Connecting to MQTT broker \""));
    Serial.print(config->_mqtt_server);
    Serial.print(':');
    Serial.print(config->_mqtt_port);
    Serial.println(F("\"..."));
#endif
    mqtt->disconnect();
    mqtt->connect();
    mqttLastConnecting = millis();
    led->setMode(LED_FADEOUT);
  }
}

static void onWifiConnect(const WiFiEventStationModeGotIP &event) {
#ifdef USE_SERIAL
  Serial.print(F("Connected to WiFi (IP: "));
  Serial.print(event.ip);
  Serial.println(')');
#endif
  wifiLastConnecting = 0;
  led->setMode(LED_FADEOUT);
  if (mqtt)
    mqttConnect();
}

static void onMqttConnect(bool sessionPresent) {
#ifdef USE_SERIAL
  Serial.println(F("Connected to MQTT broker"));
#endif
  mqttLastConnecting = 0;
  led->setMode(LED_FADEINOUT);
}

static bool mqttPublishTopic(const char *topic, const char *value) {
  if (mqtt->connected()) {
#ifdef USE_SERIAL
    Serial.print(F("Publish MQTT topic \""));
    Serial.print(topic);
    Serial.print(F("\" with value \""));
    Serial.print(value);
    Serial.println('"');
#endif

    return mqtt->publish(topic, MQTT_QOS, config->_mqtt_retained, value, 0) != 0;
  }

  return false;
}

static bool mqttPublishBarcode(const char *barcode) {
  if (mqtt && config->_mqtt_barcode_topic) {
    return mqttPublishTopic(config->_mqtt_barcode_topic, barcode);
  }

  return false;
}

static bool mqttPublishButton(btneventid_t state) {
  if (mqtt && config->_mqtt_button_topic) {
    char value[2];

    if (state == EVT_BTNCLICK)
      value[0] = '1';
    else if (state == EVT_BTNLONGCLICK)
      value[0] = '2';
    else if (state == EVT_BTNDBLCLICK)
      value[0] = '3';
    value[1] = '\0';

    return mqttPublishTopic(config->_mqtt_button_topic, value);
  }

  return false;
}

static void halt(const __FlashStringHelper *msg) {
#ifdef USE_SERIAL
  Serial.println();
  Serial.println(msg);
  Serial.println(F("System halted!"));
  Serial.flush();
#endif
  ESP.deepSleep(0);
}

static void restart() {
#ifdef USE_SERIAL
  Serial.println();
  Serial.println(F("System restarted"));
  Serial.flush();
#endif
  ESP.restart();
}

void setup() {
  Serial.begin(9600);
  Serial.println();

  if (! initSPIFFS())
    halt(F("Error initialization SPIFFS!"));
  config = new Config();
  if (! config->load()) {
    config->clear();
#ifdef USE_SERIAL
    Serial.println(F("Use default configuration"));
#endif
  }

  events = new EventQueue();
  btn = new Button(BTN_PIN, LOW, events);
  led = new Led(LED_PIN, LED_LEVEL);

  {
    bool cpNeeded = (! config->_wifi_ssid) || (! config->_mqtt_server) || (! config->_mqtt_client);

    if (! cpNeeded) {
      const uint32_t WAIT_TIME = 2000; // 2 sec.

      uint32_t start = millis();

#ifdef USE_SERIAL
      Serial.println(F("Press button during 2 sec. to start captive portal..."));
#endif
      led->setMode(LED_4HZ);
      while (millis() - start < WAIT_TIME) {
        if (events->depth()) { // Button was pressed
          cpNeeded = true;
          break;
        }
        led->delay(1);
      }
      led->setMode(LED_OFF);
    }

    if (cpNeeded) {
      CaptivePortal cp(config, led);

      btn->pause();
      cp.exec();
      events->clear();
      btn->resume();
    }
  }

  if (config->_wifi_ssid && config->_mqtt_server && config->_mqtt_client) {
    mqtt = new AsyncMqttClient();
    mqtt->setServer(config->_mqtt_server, config->_mqtt_port);
    mqtt->setClientId(config->_mqtt_client);
    if (config->_mqtt_user)
      mqtt->setCredentials(config->_mqtt_user, config->_mqtt_pswd);
    mqtt->onConnect(onMqttConnect);
  }
  WiFi.mode(WIFI_STA);
  if (config->_wifi_ssid) {
    wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  }
  barcode[0] = '\0';
#ifdef USE_SERIAL
  Serial.println(F("MQTT BarScanner started"));
#endif
}

void loop() {
  if (config->_wifi_ssid) {
    if (! WiFi.isConnected())
      wifiConnect();
    if (mqtt && WiFi.isConnected() && (! mqtt->connected()))
      mqttConnect();
  }

  {
    event_t *evt;

    while ((evt = (event_t*)events->get()) != NULL) {
      if (evt->id == EVT_BTNCLICK) {
        mqttPublishButton((btneventid_t)evt->id);
#ifdef USE_SERIAL
        Serial.println(F("Button clicked"));
#endif
      } else if (evt->id == EVT_BTNDBLCLICK) {
        mqttPublishButton((btneventid_t)evt->id);
#ifdef USE_SERIAL
        Serial.println(F("Button double clicked"));
#endif
      } else if (evt->id == EVT_BTNLONGCLICK) {
        mqttPublishButton((btneventid_t)evt->id);
#ifdef USE_SERIAL
        Serial.println(F("Button long clicked"));
#endif
/*
        config->clear();
        config->save();
#ifdef USE_SERIAL
        Serial.println(F("Configuration resets to defaults!"));
#endif
        restart();
*/
      }
    }
  }

  if (Serial.available()) {
    uint8_t codelen = strlen(barcode);

    while (Serial.available()) {
      char ch = Serial.read();

      if (ch == BARCODE_TERMINATOR) {
#ifdef USE_SERIAL
        Serial.print(F("Barcode: \""));
        Serial.print(barcode);
        Serial.println('"');
#endif
        mqttPublishBarcode(barcode);
        barcode[0] = '\0';
        codelen = 0;
      } else {
        barcode[codelen++] = ch;
        barcode[codelen] = '\0';
        if (codelen >= BARCODE_SIZE) {
#ifdef USE_SERIAL
          Serial.print(F("Barcode (cutted): \""));
          Serial.print(barcode);
          Serial.println('"');
#endif
          mqttPublishBarcode(barcode);
          barcode[0] = '\0';
          codelen = 0;
        }
      }
    }
  }

  led->delay(1);
}
