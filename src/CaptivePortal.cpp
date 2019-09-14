#ifdef ESP32
#include <SPIFFS.h>
#include <Update.h>
#else
#include <FS.h>
#include <WiFiUdp.h>
#endif
#include "CaptivePortal.h"
#include "StrUtils.h"
#include "HtmlHelper.h"

static const char INDEX_HTML[] PROGMEM = "index.html";
static const char ROOT_URI[] PROGMEM = "/";
static const char CSS_URI[] PROGMEM = "/default.css";
static const char RESTART_URI[] PROGMEM = "/restart";
static const char SPIFFS_URI[] PROGMEM = "/spiffs";
static const char FWUPDATE_URI[] PROGMEM = "/fwupdate";

static uint8_t wifiFindFreeChannel() {
  int32_t levels[MAX_WIFI_CHANNEL];
  int8_t nets, i;

  memset(levels, 0, sizeof(levels));
  WiFi.disconnect();
  nets = WiFi.scanNetworks(false, true);
  for (i = 0; i < nets; ++i) {
    uint8_t channel = WiFi.channel(i);
    int32_t rssi = WiFi.RSSI(i);

    if ((! levels[channel - 1]) || (levels[channel - 1] < rssi))
      levels[channel - 1] = rssi;
  }
  WiFi.scanDelete();

  nets = 0; // First channel
  int32_t minlevel = levels[nets] + levels[nets + 1] / 2;

  if (minlevel) {
    for (i = 1; i < MAX_WIFI_CHANNEL; ++i) {
      int32_t level = levels[i] + levels[i - 1] / 2;

      if (i < MAX_WIFI_CHANNEL - 1)
        level += levels[i + 1] / 2;
      if (! level) { // Free channel
        nets = i;
        break;
      } else if (level < minlevel) {
        minlevel = level;
        nets = i;
      }
    }
  }

  return (nets + 1);
}

bool CaptivePortal::exec() {
  {
    String _ssid = ssid();
    String _pswd = password();
    uint8_t _channel = channel();

    WiFi.mode(WIFI_AP);
#ifdef USE_SERIAL
    Serial.print(F("AP \""));
    Serial.print(_ssid);
    Serial.print(F("\" with password \""));
    Serial.print(_pswd);
    Serial.print(F("\" on channel "));
    Serial.print(_channel);
    Serial.print(F(" setup "));
#endif
    if (! WiFi.softAP(_ssid.c_str(), _pswd.c_str(), _channel)) {
#ifdef USE_SERIAL
      Serial.println(F("FAIL!"));
#endif

      return false;
    }
  }
#ifdef USE_SERIAL
  Serial.print(F("successfully (IP: "));
  Serial.print(WiFi.softAPIP());
  Serial.println(')');
#endif
  _dns = new DNSServer();
  _dns->setErrorReplyCode(DNSReplyCode::NoError);
  _dns->start(53, F("*"), WiFi.softAPIP());
#ifdef ESP32
  _http = new WebServer(80);
#else
  _http = new ESP8266WebServer(80);
#endif
  setupHandles();
  _http->begin();
#ifdef USE_SERIAL
  Serial.print(F("Visit to http://"));
  Serial.print(WiFi.softAPIP());
  Serial.println(FPSTR(ROOT_URI));
#endif

  uint32_t start = millis();

  while (millis() - start < CP_DURATION) {
    _dns->processNextRequest();
    _http->handleClient();
    if (WiFi.softAPgetStationNum()) {
      start = millis();
#ifdef USE_LED
      _led->setMode(LED_CPPROCESSING);
    } else {
      _led->setMode(LED_CPWAITING);
    }
    _led->delay(1);
#else
    }
    delay(1);
#endif
  }
#ifdef USE_LED
  _led->setMode(LED_OFF);
#endif
  _http->close();
  delete _http;
  _http = NULL;
  _dns->stop();
  delete _dns;
  _dns = NULL;
  WiFi.softAPdisconnect(true);
#ifdef USE_SERIAL
  Serial.println(F("Access Point closed"));
#endif

  return true;
}

String CaptivePortal::ssid() const {
#ifdef CP_SSID
  return String(F(CP_SSID));
#else
  String result;
  uint32_t id;
  char hex[3];

#ifdef ESP32
  id = ESP.getEfuseMac() >> 16;
#else
  id = ESP.getChipId();
#endif
  result = FPSTR(CP_PREFIX);
  for (uint8_t i = 0; i < 4; ++i) {
#ifdef ESP32
    result += byteToHex(hex, id >> (8 * i));
#else
    result += byteToHex(hex, id >> (8 * (3 - i)));
#endif
  }

  return result;
#endif
}

String CaptivePortal::password() const {
#ifdef CP_PSWD
  return String(F(CP_PSWD));
#else
  String result;
  uint32_t id;
  char hex[3];

#ifdef ESP32
  id = ESP.getEfuseMac() >> 16;
#else
  id = ESP.getChipId();
#endif
  for (uint8_t i = 0; i < 4; ++i) {
#ifdef ESP32
    result += byteToHex(hex, id >> (8 * i));
#else
    result += byteToHex(hex, id >> (8 * (3 - i)));
#endif
  }
  result += FPSTR(CP_SALT); // Password suffix

  return result;
#endif
}

uint8_t CaptivePortal::channel() const {
  return wifiFindFreeChannel();
}

void CaptivePortal::cleanup() {
#ifdef USE_SERIAL
  Serial.flush();
#endif
#ifdef USE_LED
  _led->setMode(LED_OFF);
#endif
}

void CaptivePortal::restart() {
#ifdef USE_SERIAL
  Serial.println();
  Serial.println(F("System restarting..."));
#endif
  cleanup();
  ESP.restart();
}

bool CaptivePortal::isCaptivePortal() {
  if (! _http->hostHeader().equals(WiFi.softAPIP().toString())) {
    _http->sendHeader(F("Location"), String(F("http://")) + WiFi.softAPIP().toString(), true);
    _http->send_P(302, TEXT_PLAIN, NULL, 0);

    return true;
  }

  return false;
}

void CaptivePortal::setupHandles() {
  _http->onNotFound([this]() { this->handleNotFound(); });
  _http->on(FPSTR(CSS_URI), HTTP_GET, [this]() { this->handleCss(); });
  _http->on(FPSTR(ROOT_URI), HTTP_GET, [this]() { this->handleRoot(); });
  _http->on(FPSTR(ROOT_URI), HTTP_POST, [this]() { this->handleWriteConfig(); });
  _http->on(FPSTR(RESTART_URI), HTTP_GET, [this]() { this->handleRestart(); });
  _http->on(FPSTR(SPIFFS_URI), HTTP_GET, [this]() { this->handleSPIFFS(); });
  _http->on(FPSTR(SPIFFS_URI), HTTP_POST, [this]() { this->handleFileUploaded(); }, [this]() { this->handleFileUpload(); });
  _http->on(FPSTR(SPIFFS_URI), HTTP_DELETE, [this]() { this->handleFileDelete(); });
  _http->on(FPSTR(FWUPDATE_URI), HTTP_GET, [this]() { this->handleFwUpdate(); });
  _http->on(FPSTR(FWUPDATE_URI), HTTP_POST, [this]() { this->handleSketchUpdated(); }, [this]() { this->handleSketchUpdate(); });
}

void CaptivePortal::handleNotFound() {
  if (isCaptivePortal())
    return;

#ifdef USE_AUTHORIZATION
  if (! checkAuthorization())
    return;
#endif

  if (! handleFileRead(_http->uri()))
    _http->send_P(404, TEXT_PLAIN, PSTR("Page not found!"));
}

void CaptivePortal::handleCss() {
  if (! handleFileRead(_http->uri())) {
    _http->send_P(200, TEXT_CSS, PSTR("body { background-color: rgb(240, 240, 240); }"));
  }
}

static const char TEXTAREA_NAME[] PROGMEM = "config";

void CaptivePortal::handleRoot() {
  static const char RAW_PSTR[] PROGMEM = "raw";

  if (isCaptivePortal())
    return;

#ifdef USE_AUTHORIZATION
  if (! checkAuthorization())
    return;
#endif

  if (_http->hasArg(FPSTR(RAW_PSTR))) {
    _http->send(200, FPSTR(APPLICATION_JSON), _config->toString());
  } else {
    String page = FPSTR(HTML_START);
    page += tag_P(PSTR("title"), F("Edit configuration"), true);
    page += getCss();
    page += FPSTR(HEAD_END);
    page += F("<form method=\"POST\" action=\"");
    page += FPSTR(ROOT_URI);
    page += F("\">\n");
    page += tag_P(PSTR("label"), F("JSON configuration:"));
    page += tag_P(PSTR("br"), true);
    page += F("<textarea rows=25 cols=80 name=\"");
    page += FPSTR(TEXTAREA_NAME);
    page += F("\">");
    page += _config->toString();
    page += F("</textarea><br/>\n"
      "<input type=\"SUBMIT\" value=\"Store\">\n"
      "<input type=\"RESET\" value=\"Cancel\">\n"
      "<input type=\"BUTTON\" value=\"Restart!\" onclick='location.href=\"");
    page += FPSTR(RESTART_URI);
    page += F("\"'>\n"
      "</form>\n");
    page += FPSTR(HTML_END);
    _http->send(200, FPSTR(TEXT_HTML), page);
  }
}

void CaptivePortal::handleWriteConfig() {
#ifdef USE_AUTHORIZATION
  if (! checkAuthorization())
    return;
#endif

  uint16_t code = 200;
  String page = FPSTR(HTML_START);
  page += tag_P(PSTR("title"), F("Store configuration"), true);
  page += F("<meta http-equiv=\"refresh\" content=\"2;URL=/\">\n");
  page += getCss();
  page += FPSTR(HEAD_END);
  if (_http->hasArg(FPSTR(TEXTAREA_NAME))) {
    if (_config->fromString(_http->arg(FPSTR(TEXTAREA_NAME))) && _config->save()) {
      page += F("OK");
    } else {
      page += F("Error!");
      code = 500;
    }
  } else {
    page += F("Bad arguments!");
    code = 500;
  }
  page += FPSTR(HTML_END);
  _http->send(code, FPSTR(TEXT_HTML), page);
}

void CaptivePortal::handleRestart() {
#ifdef USE_AUTHORIZATION
  if (! checkAuthorization())
    return;
#endif

  _http->send_P(200, TEXT_PLAIN, PSTR("Restarting..."));
  _http->close();

  restart();
}

void CaptivePortal::handleSPIFFS() {
#ifdef USE_AUTHORIZATION
  if (! checkAuthorization())
    return;
#endif

  String page = FPSTR(HTML_START);
  page += tag_P(PSTR("title"), F("SPIFFS"), true);
  page += F("<script type=\"text/javascript\">\n"
    "function getXmlHttpRequest() {\n"
    "var xmlhttp;\n"
    "try {\n"
    "xmlhttp = new ActiveXObject(\"Msxml2.XMLHTTP\");\n"
    "} catch (e) {\n"
    "try {\n"
    "xmlhttp = new ActiveXObject(\"Microsoft.XMLHTTP\");\n"
    "} catch (E) {\n"
    "xmlhttp = false;\n"
    "}\n"
    "}\n"
    "if ((! xmlhttp) && (typeof XMLHttpRequest != 'undefined')) {\n"
    "xmlhttp = new XMLHttpRequest();\n"
    "}\n"
    "return xmlhttp;\n"
    "}\n"
    "function openUrl(url, method) {\n"
    "var request = getXmlHttpRequest();\n"
    "request.open(method, url, false);\n"
    "request.send(null);\n"
    "if (request.status != 200)\n"
    "alert(request.responseText);\n"
    "}\n"
    "function getSelectedCount() {\n"
    "var inputs = document.getElementsByTagName(\"input\");\n"
    "var result = 0;\n"
    "for (var i = 0; i < inputs.length; i++) {\n"
    "if (inputs[i].type == \"checkbox\") {\n"
    "if (inputs[i].checked == true)\n"
    "result++;\n"
    "}\n"
    "}\n"
    "return result;\n"
    "}\n"
    "function updateSelected() {\n"
    "document.getElementsByName(\"delete\")[0].disabled = (getSelectedCount() > 0) ? false : true;\n"
    "}\n"
    "function deleteSelected() {\n"
    "var inputs = document.getElementsByTagName(\"input\");\n"
    "for (var i = 0; i < inputs.length; i++) {\n"
    "if (inputs[i].type == \"checkbox\") {\n"
    "if (inputs[i].checked == true)\n"
    "openUrl(\"");
  page += FPSTR(SPIFFS_URI);
  page += F("?filename=/\" + encodeURIComponent(inputs[i].value) + '&dummy=' + Date.now(), \"DELETE\");\n"
    "}\n"
    "}\n"
    "location.reload(true);\n"
    "}\n"
    "</script>\n");
  page += getCss();
  page += FPSTR(HEAD_END);
  page += F("<form method=\"POST\" action=\"\" enctype=\"multipart/form-data\" onsubmit=\"if (document.getElementsByName('upload')[0].files.length == 0) { alert('No file to upload!'); return false; }\">\n"
    "<h3>SPIFFS</h3>\n"
    "<p>\n");

#ifdef ESP32
  File dir = SPIFFS.open(FPSTR(ROOT_URI));
  File file;
#else
  Dir dir = SPIFFS.openDir(FPSTR(ROOT_URI));
#endif
  int cnt = 0;

#ifdef ESP32
  if (dir) {
#else
  if (dir.isDirectory()) {
#endif
    page += F("<table cols=2>\n");
#ifdef ESP32
    while (file = dir.openNextFile()) {
#else
    while (dir.next()) {
#endif
      String fileName;
      size_t fileSize;

      ++cnt;
#ifdef ESP32
      fileName = file.name();
      fileSize = file.size();
#else
      fileName = dir.fileName();
      fileSize = dir.fileSize();
#endif
      if (fileName.startsWith(FPSTR(ROOT_URI)))
        fileName = fileName.substring(1);
      page += F("<tr><td><input type=\"checkbox\" name=\"file");
      page += String(cnt);
      page += F("\" value=\"");
      page += fileName;
      page += F("\" onchange=\"updateSelected()\"><a href=\"/");
      page += fileName;
      page += F("\" download>");
      page += fileName;
      page += F("</a></td><td>");
      page += String(fileSize);
      page += F("</td></tr>\n");
    }
    page += F("</table>\n");
  }
  page += String(cnt);
  page += F(" file(s)\n"
    "<p>\n"
    "<input type=\"button\" name=\"delete\" value=\"Delete\" onclick=\"if (confirm('Are you sure to delete selected file(s)?') == true) deleteSelected()\" disabled>\n"
    "<p>\n"
    "Upload new file:<br/>\n"
    "<input type=\"file\" name=\"upload\">\n"
    "<input type=\"submit\" value=\"Upload\">\n"
    "</form>\n");
  page += FPSTR(HTML_END);
  _http->send(200, FPSTR(TEXT_HTML), page);
}

void CaptivePortal::handleFileUploaded() {
  _http->send_P(200, TEXT_HTML, PSTR("<META http-equiv=\"refresh\" content=\"2;URL=\">\n"
    "Upload successful."));
}

void CaptivePortal::handleFileUpload() {
  static File uploadFile;

  if (_http->uri() != FPSTR(SPIFFS_URI))
    return;
  HTTPUpload &upload = _http->upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    char mode[2];

    if (! filename.startsWith(FPSTR(ROOT_URI)))
      filename = '/' + filename;
    mode[0] = 'w';
    mode[1] = '\0';
    uploadFile = SPIFFS.open(filename, mode);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile)
      uploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile)
      uploadFile.close();
  }
}

void CaptivePortal::handleFileDelete() {
#ifdef USE_AUTHORIZATION
  if (! checkAuthorization())
    return;
#endif

  if (! _http->args())
    return _http->send_P(500, TEXT_PLAIN, PSTR("BAD ARGS"));

  String path = _http->arg(0);
  if (path == FPSTR(ROOT_URI))
    return _http->send_P(500, TEXT_PLAIN, PSTR("BAD PATH"));
  if (! SPIFFS.exists(path))
    return _http->send_P(404, TEXT_PLAIN, PSTR("File not found!"));
  SPIFFS.remove(path);
  _http->send_P(200, TEXT_PLAIN, PSTR("OK"));
}

void CaptivePortal::handleFwUpdate() {
#ifdef USE_AUTHORIZATION
  if (! checkAuthorization())
    return;
#endif

  String page = FPSTR(HTML_START);
  page += tag_P(PSTR("title"), F("Sketch Update"), true);
  page += getCss();
  page += FPSTR(HEAD_END);
  page += F("<form method=\"POST\" action=\"\" enctype=\"multipart/form-data\" onsubmit=\"if (document.getElementsByName('update')[0].files.length == 0) { alert('No file to update!'); return false; }\">\n"
    "Select compiled sketch to upload:<br/>\n"
    "<input type=\"file\" name=\"upload\">\n"
    "<input type=\"submit\" value=\"Update\">\n"
    "</form>\n");
  page += FPSTR(HTML_END);
  _http->send(200, FPSTR(TEXT_HTML), page);
}

void CaptivePortal::handleSketchUpdated() {
  _http->send_P(200, TEXT_HTML, Update.hasError() ? PSTR("Update failed!") : PSTR("<META http-equiv=\"refresh\" content=\"15;URL=\">\nUpdate successful! Rebooting..."));
  if (! Update.hasError()) {
    _http->close();
    restart();
  }
}

void CaptivePortal::handleSketchUpdate() {
  if (_http->uri() != FPSTR(FWUPDATE_URI))
    return;

  HTTPUpload &upload = _http->upload();
  if (upload.status == UPLOAD_FILE_START) {
//    cleanup();
#ifndef ESP32
    WiFiUDP::stopAll();
#endif
#ifdef USE_SERIAL
    Serial.print(F("Update sketch from file \""));
    Serial.print(upload.filename);
    Serial.print('"');
#endif
    if (! Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) { // start with max available size
#ifdef USE_SERIAL
      Serial.println();
      Update.printError(Serial);
#endif
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
#ifdef USE_SERIAL
    Serial.print('.');
#endif
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
#ifdef USE_SERIAL
      Serial.println();
      Update.printError(Serial);
#endif
    }
  } else if (upload.status == UPLOAD_FILE_END) {
#ifdef USE_SERIAL
    Serial.println();
#endif
    if (Update.end(true)) { // true to set the size to the current progress
#ifdef USE_SERIAL
      Serial.print(F("Updated "));
      Serial.print(upload.totalSize);
      Serial.println(F(" byte(s) successful"));
#endif
    } else {
#ifdef USE_SERIAL
      Update.printError(Serial);
#endif
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.end();
#ifdef USE_SERIAL
    Serial.println();
    Serial.println(F("Update was aborted!"));
#endif
  }
  yield();
}

#ifdef USE_AUTHORIZATION
bool CaptivePortal::checkAuthorization() {
  char user[sizeof(AUTH_USER)];
  char pswd[sizeof(AUTH_PSWD)];

  strcpy_P(user, PSTR(AUTH_USER));
  strcpy_P(pswd, PSTR(AUTH_PSWD));
  if (! _http->authenticate(user, pswd)) {
    _http->requestAuthentication();

    return false;
  }

  return true;
}
#endif

String CaptivePortal::getContentType(const String &fileName) {
  if (_http->hasArg(F("download")))
    return String(F("application/octet-stream"));
  else if (fileName.endsWith(F(".htm")) || fileName.endsWith(F(".html")))
    return String(FPSTR(TEXT_HTML));
  else if (fileName.endsWith(F(".css")))
    return String(FPSTR(TEXT_CSS));
  else if (fileName.endsWith(F(".js")))
    return String(F("application/javascript"));
  else if (fileName.endsWith(F(".png")))
    return String(F("image/png"));
  else if (fileName.endsWith(F(".gif")))
    return String(F("image/gif"));
  else if (fileName.endsWith(F(".jpg")) || fileName.endsWith(F(".jpeg")))
    return String(F("image/jpeg"));
  else if (fileName.endsWith(F(".ico")))
    return String(F("image/x-icon"));
  else if (fileName.endsWith(F(".xml")))
    return String(F("text/xml"));
  else if (fileName.endsWith(F(".pdf")))
    return String(F("application/x-pdf"));
  else if (fileName.endsWith(F(".zip")))
    return String(F("application/x-zip"));
  else if (fileName.endsWith(F(".gz")))
    return String(F("application/x-gzip"));

  return String(FPSTR(TEXT_PLAIN));
}

bool CaptivePortal::handleFileRead(const String &path) {
  String fileName = path;

  if (fileName.endsWith(FPSTR(ROOT_URI)))
    fileName += FPSTR(INDEX_HTML);
  String contentType = getContentType(fileName);
  if (SPIFFS.exists(fileName)) {
    char mode[2];

    mode[0] = 'r';
    mode[1] = '\0';
    File file = SPIFFS.open(fileName, mode);
    if (file) {
      _http->streamFile(file, contentType);
      file.close();

      return true;
    }
  }

  return false;
}

String CaptivePortal::getCss() {
  String result = F("<link rel=\"stylesheet\" href=\"");
  result += FPSTR(CSS_URI);
  result += F("\">\n");

  return result;
}
