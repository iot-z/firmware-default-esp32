/**
 * ModuleCore.cpp
 * @author: Renan Vaz <renan.c.vaz@gmail.com>
 */

#include "ModuleCore.h"

long _startPressReset;

struct ModesStruct {
  char CONFIG = '0';
  char SLAVE = '1';
} Modes;

WebServer _webServer(80);

Socket _socket;


void taskModule (void * _module)
{
  for(;;) {
    ModuleCore::getInstance().loop();

    vTaskDelay(1);
  }

  vTaskDelete(NULL);
}

void taskResetButton(void * _module)
{
  ModuleCore::getInstance().setupResetButton();

  for(;;) {
    ModuleCore::getInstance().loopResetButton();

    vTaskDelay(1);
  }

  vTaskDelete(NULL);
}

void taskOta(void * _module)
{
  for(;;) {
    ModuleCore::getInstance().loopOta();

    vTaskDelay(1);
  }

  vTaskDelete(NULL);
}

ModuleCore::ModuleCore()
{
}

void ModuleCore::setup(String& id, String& type, String& version)
{
  _id = id;
  _type = type;
  _version = version;

  Config.load();

  if (_isFirstBoot()) {
    #ifdef MODULE_CAN_DEBUG
      Serial.println("First boot. Formating...");
    #endif

    Config.clear(); // Clear EEPROM data
    Config.setDeviceMode(Modes.CONFIG);

    #ifdef MODULE_CAN_DEBUG
      Serial.println("Restarting...");
    #endif

    ESP.restart();
  }

  xTaskCreatePinnedToCore(taskResetButton, "taskResetButton", 1024 * 4, NULL, 2, NULL, 1);

  if (isConfigMode()) {
    _setupConfigMode();
  } else {
    _setupSlaveMode();
  }

  xTaskCreatePinnedToCore(taskModule, "  ", 1024 * 16, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(taskOta, "taskOta", 1024 * 4, NULL, 1, NULL, 1);
}

void ModuleCore::loop()
{
  if (isConfigMode()) {
    _loopConfigMode();
  } else {
    _loopSlaveMode();
  }
}

// OTA
void ModuleCore::setupOta()
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

    #ifdef MODULE_CAN_DEBUG
      Serial.println("Iniciando OTA - " + s);
    #endif
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
      Serial.println("Erro " + String(error) + " ");
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

void ModuleCore::loopOta()
{
  ArduinoOTA.handle();
}

// Button
void ModuleCore::setupResetButton()
{
  pinMode(_resetButtonPin, INPUT_PULLUP);
}

void ModuleCore::loopResetButton()
{
  bool isPressed = !digitalRead(_resetButtonPin);

  if (isPressed) {
    if (_startPressReset == 0) {
      _startPressReset = xTaskGetTickCount() / portTICK_PERIOD_MS;
    }
  } else if (_startPressReset > 0) {
    uint16_t holdTime = (uint16_t) ((xTaskGetTickCount() / portTICK_PERIOD_MS) - _startPressReset);

    _startPressReset = 0;

    if (holdTime > 15000) {
      #ifdef MODULE_CAN_DEBUG
        Serial.println("Long button reset press (15s). Clear data.");
      #endif

      Config.clear();
    } else if (holdTime > 5000) {
      #ifdef MODULE_CAN_DEBUG
        Serial.println("Long button reset press (5s). Config mode.");
      #endif

      Config.setDeviceMode(Modes.CONFIG);
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

// Cofig mode
void ModuleCore::_setupConfigMode()
{
  _setupConfigMode_wifi();
  _setupConfigMode_webServer();
}

void ModuleCore::_loopConfigMode()
{
  _loopConfigMode_webServer();
}

void ModuleCore::_setupConfigMode_wifi()
{
  String deviceName = Config.getDeviceName();

  String ssid = "IOTZ - ";
  ssid += _type;
  ssid += " (";
  ssid += deviceName[0] != '\0' ? deviceName : String(random(0xffff), HEX);
  ssid += ")";

  WiFi.setHostname("iotz.local");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid.c_str(), _apPassword.c_str());

  #ifdef MODULE_CAN_DEBUG
    Serial.println("SSID: ");
    Serial.print(ssid);
    Serial.println("PASS: ");
    Serial.print(_apPassword);
    Serial.println("Local server address: ");
    Serial.print(WiFi.softAPIP());
  #endif
}

void ModuleCore::_setupConfigMode_webServer()
{
  // Init SPIFFS for load the index.html file
  SPIFFS.begin();

  // HTML da pagina principal
  File fileIndex = SPIFFS.open("/index.html", "r");
  File fileSuccess = SPIFFS.open("/success.html", "r");

  if (fileIndex) {
    _htmlIndex = fileIndex.readString();
  } else {
    #ifdef MODULE_CAN_DEBUG
      Serial.println("ERROR on loading \"index.html\" file");
    #endif
  }

  if (fileSuccess) {
    _htmlSuccess = fileSuccess.readString();
  } else {
    #ifdef MODULE_CAN_DEBUG
      Serial.println("ERROR on loading \"success.html\" file");
    #endif
  }

  // Start the server
  _webServer.on("/", HTTP_GET, [&]() {
    _webServer.send(200, "text/html", _parseHTML(_htmlIndex));
  });

  _webServer.on("/", HTTP_POST, [&]() {
    String deviceName = _webServer.arg("device-name");
    String ssid       = _webServer.arg("ssid");
    String password   = _webServer.arg("password");
    String serverIp   = _webServer.arg("server-ip");
    String serverPort = _webServer.arg("server-port");

    #ifdef MODULE_CAN_DEBUG
      Serial.println("Device name: "+deviceName);
      Serial.println("Password: "+password);
      Serial.println("Server address: "+serverIp+":"+serverPort);
    #endif

    Config.setDeviceName(deviceName);
    Config.setNetworkSsid(ssid);
    Config.setNetworkPassword(password);
    Config.setServerIp(serverIp);
    Config.setServerPort(serverPort);
    Config.setDeviceMode(Modes.SLAVE);

    _webServer.send(200, "text/html", _parseHTML(_htmlSuccess));

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    #ifdef MODULE_CAN_DEBUG
      Serial.println("Restarting...");
    #endif

    ESP.restart();
  });

  _webServer.begin();
}

void ModuleCore::_loopConfigMode_webServer()
{
  _webServer.handleClient();
}

// Slave mode
void ModuleCore::_setupSlaveMode()
{
  _setupSlaveMode_wifi();

  on("setDeviceName", [](const JsonObject &in, const JsonObject &out) {
    String deviceName = in["deviceName"];

    Config.setDeviceName(deviceName);
  });

  IPAddress serverIp; serverIp.fromString(Config.getServerIp());
  uint16_t serverPort = Config.getServerPort().toInt();

  _socket.onMessage([&](const JsonObject &message) {
    _onMessage(message);
  });

  _socket.onConnected([&]() {
    _onConnected();
  });

  _socket.connect(serverIp, serverPort);
}

void ModuleCore::_setupSlaveMode_wifi()
{
  String ssid = Config.getNetworkSsid().c_str();
  String password = Config.getNetworkPassword().c_str();

  #ifdef MODULE_CAN_DEBUG
    Serial.println("Try to connect to: ");
    Serial.print(ssid);
    Serial.println("With password: ");
    Serial.print(password);
  #endif

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    #ifdef MODULE_CAN_DEBUG
      Serial.print(".");
    #endif

    vTaskDelay(500 / portTICK_PERIOD_MS);
  }

  #ifdef MODULE_CAN_DEBUG
    Serial.println("");
    Serial.print("Connected to: ");
    Serial.println(Config.getNetworkSsid());
  #endif
}

void ModuleCore::_loopSlaveMode()
{
  _socket.loop();
}

// Events
void ModuleCore::send(const char* topic)
{
  StaticJsonDocument<PACKET_SIZE> doc;
  JsonObject data = doc.to<JsonObject>();

  send(topic, data);
}

void ModuleCore::send(const char* topic, const JsonObject &data)
{
  StaticJsonDocument<PACKET_SIZE> doc;
  JsonObject message = doc.to<JsonObject>();

  message["moduleId"] = _id;
  message["topic"]    = topic;
  message["data"]     = data;

  _socket.send(message);
}

void ModuleCore::on(String topic, std::function<void(const JsonObject &in, const JsonObject &out)> listener)
{
  #ifdef MODULE_CAN_DEBUG
    Serial.println("Add listener: "+topic);
  #endif

  _events[topic] = listener;
}

void ModuleCore::_onConnected() {
  StaticJsonDocument<PACKET_SIZE> doc;
  JsonObject data = doc.to<JsonObject>();

  data["name"]     = Config.getDeviceName();
  data["type"]     = _type;
  data["version"]  = _version;

  #ifdef MODULE_CAN_DEBUG
  Serial.println("onConnected");
  #endif

  send("_identify", data);
}

void ModuleCore::_onMessage(const JsonObject &message) {
  StaticJsonDocument<PACKET_SIZE> doc;
  JsonObject outData = doc.to<JsonObject>();

  String topic = message["topic"];
  const JsonObject data = message["data"];

  if (_events.count(topic)) {
    _events.at(topic)(data, outData);
  } else {
    #ifdef MODULE_CAN_DEBUG
      Serial.println("Topic not found: "+topic);
    #endif
  }

  if (message.containsKey("_")) {
    const char* replyTopic = message["_"];

    send(replyTopic, outData);
  }
}

void ModuleCore::createArduinoApi()
{
  on("pinMode", [](const JsonObject &in, const JsonObject &out) {
    uint8_t pin = in["pin"];
    String mode = in["mode"];

    pinMode(pin, mode == "OUTPUT" ? OUTPUT : INPUT);
  });

  on("digitalWrite", [](const JsonObject &in, const JsonObject &out) {
    uint8_t pin  = in["pin"];
    String level = in["level"];

    #ifdef MODULE_CAN_DEBUG
      Serial.println(pin);
      Serial.println(level);
    #endif

    digitalWrite(pin, level == "1" ? HIGH : LOW);
  });

  on("digitalRead", [](const JsonObject &in, const JsonObject &out) {
    uint8_t pin    = in["pin"];
    uint8_t level  = digitalRead(pin);

    out["level"] = level;
  });
}

void ModuleCore::setApPassword(String password)
{
  _apPassword = password;
}

void ModuleCore::setResetButtonPin(uint16_t pin)
{
  _resetButtonPin = pin;
}

void ModuleCore::setLedStatusPin(uint16_t pin)
{
  _ledStatusPin = pin;
}

bool ModuleCore::_isFirstBoot()
{
  return Config.getDeviceMode() != Modes.CONFIG && Config.getDeviceMode() != Modes.SLAVE;
}

bool ModuleCore::isConfigMode()
{
  return Config.getDeviceMode() == Modes.CONFIG;
}

String ModuleCore::getId()
{
  return _id;
}

String ModuleCore::getType()
{
  return _type;
}

String ModuleCore::getVersion()
{
  return _version;
}

String ModuleCore::_parseHTML(String html)
{
  html.replace("{{ device-id }}", _id);
  html.replace("{{ device-type }}", _type);
  html.replace("{{ firmware-version }}", _version);
  html.replace("{{ device-name }}", Config.getDeviceName());
  html.replace("{{ server-ip }}", Config.getServerIp());
  html.replace("{{ server-port }}", Config.getServerPort());
  html.replace("{{ ssid }}", Config.getNetworkSsid());
  html.replace("{{ password }}", Config.getNetworkPassword());

  return html;
}