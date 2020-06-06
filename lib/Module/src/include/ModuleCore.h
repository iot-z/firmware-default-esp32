/**
 * ModuleCore.h
 * @author: Renan Vaz <renan.c.vaz@gmail.com>
 */

#ifndef ModuleCore_h
#define ModuleCoree_h

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <cstdint>

#include <ArduinoOTA.h>

#include "Modes.h"
#include "Config.h"

#include "ModeSlave.h"

class ModuleCore
{
  public:
    ModuleCore();

    std::string getId();
    std::string getType();
    std::string getVersion();

    void setup(std::string& id, std::string& type, std::string& version);
    void loop();

    void send(const char* topic);
    void send(const char* topic, JsonObject& data);

    void on(const char* eventName, std::function<void(JsonObject&, JsonObject&)> cb);

    void setResetButtonPin(uint16_t pin);
    void setLedStatusPin(uint16_t pin);
    void setApPassword(std::string);

    void createArduinoApi();

    bool isConfigMode();

  private:
    std::string _id;
    std::string _type;
    std::string _version;

    std::map<std::string, std::function<void(JsonObject& in, JsonObject& out)>> _events = 0;

    uint8_t _resetButtonPin = D1;
    uint8_t _ledStatusPin = LED_BUILTIN;
    uint8_t _apPassword = "123456789";

    WebServer* _webServer;

    void _setupOta();
    void _setupConfigMode();
    void _setupSlaveMode();

    void _setupConfigMode_wifi();
    void _setupConfigMode_webServer();

    void _setupSlaveMode_wifi();

    void _setupResetButton();
    void _loopResetButton();

    bool _isFirstBoot();

    std::string _parseHTML();


    void _onMessage(const char* topic, JsonObject& in, JsonObject& out);
};

#endif
