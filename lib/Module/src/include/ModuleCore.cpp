/**
 * ModuleCore.cpp
 * @author: Renan Vaz <renan.c.vaz@gmail.com>
 */

#include "ModuleCore.h"

long _startPressReset;

const enum Modes :char {
  CONFIG = '0',
  SLAVE = '1',
};

ModuleCore::ModuleCore()
{
}

void ModuleCore::setup(String& id, String& type, String& version)
{
  _id = id;
  _type = type;
  _version = version;

  Config.load();

  _setupResetButton();

  if (_isFirstBoot()) {
    #ifdef MODULE_CAN_DEBUG
      Serial.println("First boot. Formating...");
    #endif

    Config.clear(); // Clear EEPROM data
    Config.setDeviceMode(Modes::CONFIG);

    #ifdef MODULE_CAN_DEBUG
      Serial.println("Restarting...");
    #endif

    ESP.restart();
  }

  if (isConfigMode()) {
    _setupModeConfig();
  } else {
    _setupModeSlave();
  }
}

void ModuleCore::loop()
{
  _loopResetButton();

  if (isConfigMode()) {
    _loopModeConfig();
  } else {
    _loopModeSlave();
  }
}

void _setupOta()
{
  ArduinoOTA.setPort(3232);
  ArduinoOTA.setHostname("iotz.ota");

  ArduinoOTA.onStart([](){
    String s;

    if (ArduinoOTA.getCommand() == U_FLASH) {
      // Atualizar sketch
      s = "Sketch";
    } else { // U_SPIFFS
      // Atualizar SPIFFS
      s = "SPIFFS";
      // SPIFFS deve ser finalizada
      SPIFFS.end();
    }

    Serial.println("Iniciando OTA - " + s);
  });

  // Fim
  ArduinoOTA.onEnd([](){
    #ifdef MODULE_CAN_DEBUG
      Serial.println("OTA Concluído.");
    #endif
  });

  // Progresso
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    #ifdef MODULE_CAN_DEBUG
      Serial.print(progress * 100 / total);
      Serial.print(" ");
    #endif
  });

  // Falha
  ArduinoOTA.onError([](ota_error_t error) {
    #ifdef MODULE_CAN_DEBUG
      Serial.print("Erro " + String(error) + " ");
    #endif

    if (error == OTA_AUTH_ERROR) {
      #ifdef MODULE_CAN_DEBUG
        Serial.println("Falha de autorização");
      #endif
    } else if (error == OTA_BEGIN_ERROR) {
      #ifdef MODULE_CAN_DEBUG
        Serial.println("Falha de inicialização");
      #endif
    } else if (error == OTA_CONNECT_ERROR) {
      #ifdef MODULE_CAN_DEBUG
        Serial.println("Falha de conexão");
      #endif
    } else if (error == OTA_RECEIVE_ERROR) {
      #ifdef MODULE_CAN_DEBUG
        Serial.println("Falha de recebimento");
      #endif
    } else if (error == OTA_END_ERROR) {
      #ifdef MODULE_CAN_DEBUG
        Serial.println("Falha de finalização");
      #endif
    } else {
      #ifdef MODULE_CAN_DEBUG
        Serial.println("Falha desconhecida");
      #endif
    }
  });

  // Inicializa OTA
  ArduinoOTA.begin();
}

// Cofig mode
void ModuleCore::_setupConfigMode()
{
  _setupConfigMode_wifi();
  _setupConfigMode_webServer();
}

void ModuleCore::_setupConfigMode_wifi()
{
  String deviceName = Config.getDeviceName();

  String ssid = "Module - ";
  ssid += Device.TYPE;
  ssid += " (";
  ssid += deviceName[0] != '\0' ? deviceName : String("Unamed ") + String(random(0xffff), HEX);
  ssid += ")";

  WiFi.setHostname("iotz.local");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, _apPassword);

  #ifdef MODULE_CAN_DEBUG
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("PASS: ");
    Serial.println(_apPassword);
    Serial.print("Local server address: ");
    Serial.println(WiFi.softAPIP());
  #endif
}

void ModuleCore::_setupConfigMode_webServer()
{
  String htmlIndex;
  String htmlSuccess;

  server = new WebServer(80);

  // Init SPIFFS for load the index.html file
  SPIFFS.begin();

  // HTML da pagina principal
  File fileIndex = SPIFFS.open("/index.html", "r");
  File fileSuccess = SPIFFS.open("/success.html", "r");

  if (fileIndex) {
    htmlIndex = fileIndex.readString();
  } else {
    #ifdef MODULE_CAN_DEBUG
      Serial.println("ERROR on loading \"index.html\" file");
    #endif
  }

  if (fileSuccess) {
    htmlSuccess = fileSuccess.readString();
  } else {
    #ifdef MODULE_CAN_DEBUG
      Serial.println("ERROR on loading \"success.html\" file");
    #endif
  }

  // Start the server
  _webServer->on("/", HTTP_GET, std::bind([&htmlIndex]() {
    _webServer->send(200, "text/html", _parseHTML(htmlIndex));
  });

  _webServer->on("/", HTTP_POST, [&htmlSuccess]() {
    String deviceName = _webServer->arg("device-name");
    String ssid       = _webServer->arg("ssid");
    String password   = _webServer->arg("password");
    String serverIp   = _webServer->arg("server-ip");
    String serverPort = _webServer->arg("server-port");

    #ifdef MODULE_CAN_DEBUG
      Serial.print("Device name: ");
      Serial.println(deviceName);
      Serial.print("SSID:");
      Serial.println(ssid);
      Serial.print("Password:");
      Serial.println(password);
    #endif

    Config.setDeviceName(deviceName);
    Config.setNetworkSsid(ssid);
    Config.setNetworkPassword(password);
    Config.setServerIp(serverIp);
    Config.setServerPort(serverPort);
    Config.setDeviceMode(Modes::SLAVE);

    _webServer->send(200, "text/html", _parseHTML(htmlSuccess));

    #ifdef MODULE_CAN_DEBUG
      Serial.println("Restarting...");
    #endif

    ESP.restart();
  });

  _webServer->begin();
}

// Slave mode
void ModuleCore::_setupSlaveMode()
{
  _setupSlaveMode_wifi();

  on("setDeviceName", [](JsonObject& in, JsonObject& out) {
    String deviceName = in["deviceName"];

    Config.setDeviceName(deviceName);
  });
}

void ModuleCore::_setupSlaveMode_wifi()
{
  bool error = false;

  IPAddress ip; ip.fromString(Config.getServerIp());
  uint16_t port = Config.getServerPort().toInt();

  #ifdef MODULE_CAN_DEBUG
    Serial.print("Try to connect to: ");
    Serial.println(Config.getNetworkSsid());
    Serial.print("With password: ");
    Serial.println(Config.getNetworkPassword());
  #endif

  WiFi.mode(WIFI_STA);
  WiFi.begin(Config.dataNetworkSsid(), Config.dataNetworkPassword());

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    #ifdef MODULE_CAN_DEBUG
      Serial.print(".");
    #endif

    delay(500);
  }

  #ifdef MODULE_CAN_DEBUG
    Serial.println();
    Serial.print("Connected to: ");
    Serial.println(Config.getNetworkSsid());
  #endif
}


// Button
void ModuleCore::_setupResetButton()
{
  pinMode(_resetButtonPin, INPUT_PULLDOWN);
}

void ModuleCore::_loopResetButton()
{
  bool isPressed = digitaRead(RESET_BUTTON_PIN);

  if (isPressed) {
    _startPressReset = millis()
  } else if (_startPressReset) {
    uint16_t holdTime = (uint16_t) (millis() - _startPressReset);

    _startPressReset = NULL;

    if (holdTime > 15000) {
      #ifdef MODULE_CAN_DEBUG
        Serial.println("Long button reset press (15s). Clear data.");
      #endif

      Config.clear();
    } else if (holdTime > 5000) {
      #ifdef MODULE_CAN_DEBUG
        Serial.println("Long button reset press (5s). Config mode.");
      #endif

      Config.setDeviceMode(Modes::CONFIG);
    } else {
      #ifdef MODULE_CAN_DEBUG
        Serial.println("Button reset press.");
      #endif
    }

    #ifdef MODULE_CAN_DEBUG
      Serial.println("Restarting...");
    #endif

    ESP.restart();
  }
}

void ModuleCore::send(const char* topic)
{
  _modeSlave->send(topic);
}

void ModuleCore::send(const char* topic, JsonObject& data)
{
  _modeSlave->send(topic, data);
}

void ModuleCore::on(const char* eventName, std::function<void(JsonObject& in, JsonObject& out)> cb)
{
  _modeSlave->on(eventName, cb);
}

void ModuleCore::createArduinoAPI()
{
  on("pinMode", [](JsonObject& in, JsonObject& out) {
    uint8_t pin = in["pin"];
    String mode = in["mode"];

    pinMode(pin, mode == "OUTPUT" ? OUTPUT : INPUT);
  });

  on("digitalWrite", [](JsonObject& in, JsonObject& out) {
    uint8_t pin  = in["pin"];
    String level = in["level"];
    Serial.println(pin);
    Serial.println(level);

    digitalWrite(pin, level == "1" ? HIGH : LOW);
  });

  on("digitalRead", [](JsonObject& in, JsonObject& out) {
    uint8_t pin    = in["pin"];
    uint8_t level  = digitalRead(pin);

    out["level"] = level;
  });
}

bool ModuleCore::setApPassword(String password)
{
  _apPassword = password;
}

bool ModuleCore::setResetButtonPin(uint16_t pin)
{
  _resetButtonPin = pin;
}

bool ModuleCore::setLedStatusPin(uint16_t pin)
{
  _ledStatusPin = pin;
}

bool ModuleCore::isFirstBoot()
{
  return Config.getDeviceMode() != '0' && Config.getDeviceMode() != '1';
}

bool ModuleCore::isConfigMode()
{
  return Config.getDeviceMode() == '0';
}

String ModuleCore::getId() {
  return _id;
}

String ModuleCore::getType() {
  return _type;
}

String ModuleCore::getVersion() {
  return _version;
}

String ModuleCore::_parseHTML(String html)
{
  html.replace("{{ device-id }}", id);
  html.replace("{{ device-type }}", type);
  html.replace("{{ firmware-version }}", version);
  html.replace("{{ device-name }}", Config.getDeviceName());
  html.replace("{{ server-ip }}", Config.getServerIp());
  html.replace("{{ server-port }}", Config.getServerPort());
  html.replace("{{ ssid }}", Config.getNetworkSsid());
  html.replace("{{ password }}", Config.getNetworkPassword());

  return html;
}