/**
 * Config.h
 * @author: Renan Vaz <renan.c.vaz@gmail.com>
 */

#ifndef Config_h
#define Config_h

#include <Arduino.h>
#include <EEPROM.h>
#include <cstdint>

// EEPROM memory address
#define ADDRESS_CONFIG 0
#define EEPROM_SIZE 512

struct DataStruct
{
  char deviceMode[2];
  char deviceName[33];
  char networkSsid[33];
  char networkPassword[64];
  char serverIp[16]; // uint8_t serverIp[4];
  char serverPort[6]; //uint16_t serverPort;
};

class ConfigClass
{
  public:
    void load();
    void save();
    void clear();

    void setDeviceMode(char);
    char getDeviceMode();

    void setDeviceName(String);
    String getDeviceName();

    void setNetworkSsid(String);
    String getNetworkSsid();

    void setNetworkPassword(String);
    String getNetworkPassword();

    void setServerIp(String);
    String getServerIp();

    void setServerPort(String);
    String getServerPort();

  private:
    DataStruct _data;
};

extern ConfigClass Config;

#endif
