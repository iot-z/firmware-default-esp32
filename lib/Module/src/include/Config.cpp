/**
 * Config.cpp
 * @author: Renan Vaz <renan.c.vaz@gmail.com>
 */

#include "Config.h"

void ConfigClass::load()
{
    EEPROM.begin(EEPROM_SIZE);

    for (uint16_t i = 0, l = sizeof(_data); i < l; i++){
    *((char*)&_data + i) = EEPROM.read(ADDRESS_CONFIG + i);
    }

    EEPROM.end();
}

void ConfigClass::save()
{
    EEPROM.begin(EEPROM_SIZE);

    for (uint16_t i = 0, l = sizeof(_data); i < l; i++){
        EEPROM.write(ADDRESS_CONFIG + i, *((char*)&_data + i));
    }

    EEPROM.end();
}

void ConfigClass::clear()
{
    #ifdef MODULE_CAN_DEBUG
    Serial.println("Cleaning data...");
    #endif

    for (uint16_t i = 0, l = sizeof(_data); i < l; i++){
        *((char*)&_data + i) = '\0';
    }

    save();
}

void ConfigClass::setDeviceMode(char value)
{
    strcpy(_data.deviceMode, String(value).c_str());

    save();
}

char ConfigClass::getDeviceMode()
{
    return _data.deviceMode[0];
}

void ConfigClass::setDeviceName(String value)
{
    strcpy(_data.deviceName, value.c_str());

    save();
}

String ConfigClass::getDeviceName()
{
    return String(_data.deviceName);
}

void ConfigClass::setNetworkSsid(String value)
{
    strcpy(_data.networkSsid, value.c_str());

    save();
}

String ConfigClass::getNetworkSsid()
{
    return String(_data.networkSsid);
}

void ConfigClass::setNetworkPassword(String value)
{
    strcpy(_data.networkPassword, value.c_str());

    save();
}

String ConfigClass::getNetworkPassword()
{
    return String(_data.networkPassword);
}

void ConfigClass::setServerIp(String value)
{
    strcpy(_data.serverIp, value.c_str());

    save();
}

String ConfigClass::getServerIp()
{
    return String(_data.serverIp);
}

void ConfigClass::setServerPort(String value)
{
    strcpy(_data.serverPort, value.c_str());

    save();
}

String ConfigClass::getServerPort()
{
    return String(_data.serverPort);
}

ConfigClass Config;
