/**
 * ModeConfig.h
 * @author: Renan Vaz <renan.c.vaz@gmail.com>
 */

#ifndef ModeConfig_h
#define ModeConfig_h

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <FS.h>
#include <cstdint>

#include "Modes.h"
#include "Device.h"
#include "Config.h"

class ModeConfig
{
  public:
    ModeConfig();

    String ID;
    String TYPE;
    String VERSION;

    void setup();
    void loop();

  private:
    ESP8266WebServer* server;

    // HTML data for config mode
    String _htmlRoot;
    String _htmlSuccess;

    void _handleRootGET();
    void _handleRootPOST();

    String _parseHTML(String html);
};

#endif
