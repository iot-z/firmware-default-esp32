/**
 * ModuleCore.h
 * @author: Renan Vaz <renan.c.vaz@gmail.com>
 */

#ifndef ModuleCore_h
#define ModuleCore_h

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <map>

#include <ArduinoOTA.h>

#include "Config.h"
#include "Socket.h"

class ModuleCore
{
  public:
    ModuleCore();

    String getId();
    String getType();
    String getVersion();

    static ModuleCore& getInstance()
    {
      static ModuleCore instance;
      return instance;
    }

    void print(String& message);
    void setup(String& id, String& type, String& version);
    void loop();

    void setupOta();
    void loopOta();

    void setupResetButton();
    void loopResetButton();

    void send(const char* topic);
    void send(const char* topic, const JsonObject &data);

    void on(String topic, std::function<void(const JsonObject &in, const JsonObject &out)> cb);

    void setResetButtonPin(uint16_t pin);
    void setLedStatusPin(uint16_t pin);
    void setApPassword(String);

    void createArduinoApi();

    bool isConfigMode();

  private:
    uint8_t _resetButtonPin = 15; // D15
    uint8_t _ledStatusPin = LED_BUILTIN;
    String _apPassword = "123456789";

    String _id;
    String _type;
    String _version;

    String _htmlIndex;
    String _htmlSuccess;

    std::map<String, std::function<void(const JsonObject &in, const JsonObject &out)>> _events;

    void _setupConfigMode();
    void _loopConfigMode();

    void _setupSlaveMode();
    void _loopSlaveMode();

    void _setupConfigMode_wifi();

    void _setupConfigMode_webServer();
    void _loopConfigMode_webServer();

    void _setupSlaveMode_wifi();

    bool _isFirstBoot();

    String _parseHTML(String html);

    void _onMessage(const JsonObject &message);
    void _onConnected();
};

#endif
