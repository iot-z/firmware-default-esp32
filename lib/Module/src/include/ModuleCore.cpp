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


void  ModuleCore (void * _module)
{
  ModuleCore module = ModuleCore::getInstance();

  for(;;) {
    module.loop();

    vTaskDelay(1);
  }

  vTaskDelete(NULL);
}

void taskResetButton(void * _module)
{
  ModuleCore module = ModuleCore::getInstance();

  module.setupResetButton();

  for(;;) {
    module.loopResetButton();

    vTaskDelay(1);
  }

  vTaskDelete(NULL);
}

void taskOta(void * _module)
{
  ModuleCore module = ModuleCore::getInstance();

  module.setupOta();

  for(;;) {
    module.loopOta();

    vTaskDelay(1);
  }

  vTaskDelete(NULL);
}

ModuleCore::ModuleCore()
{
}

void ModuleCore::print(String message)
{
  print(String(message));
}


void ModuleCore::print(String& message)
{
  #ifdef MODULE_CAN_DEBUG
    Serial.println(message);
  #endif
}

void ModuleCore::setup(String& id, String& type, String& version)
{
  _id = id;
  _type = type;
  _version = version;

  Config.load();

  if (_isFirstBoot()) {
    print("First boot. Formating...");

    Config.clear(); // Clear EEPROM data
    Config.setDeviceMode(Modes.CONFIG);

    print("Restarting...");

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

    print("Iniciando OTA - " + s);
  });

  // Fim
  ArduinoOTA.onEnd([](){
    print("OTA Concluído.");
  });

  // Progresso
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    print(progress * 100 / total);
    print(" ");
  });

  // Falha
  ArduinoOTA.onError([](ota_error_t error) {
    print("Erro " + String(error) + " ");

    if (error == OTA_AUTH_ERROR) {
      print("Falha de autorização");
    } else if (error == OTA_BEGIN_ERROR) {
      print("Falha de inicialização");
    } else if (error == OTA_CONNECT_ERROR) {
      print("Falha de conexão");
    } else if (error == OTA_RECEIVE_ERROR) {
      print("Falha de recebimento");
    } else if (error == OTA_END_ERROR) {
      print("Falha de finalização");
    } else {
      print("Falha desconhecida");
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
      print("Long button reset press (15s). Clear data.");

      Config.clear();
    } else if (holdTime > 5000) {
      print("Long button reset press (5s). Config mode.");

      Config.setDeviceMode(Modes.CONFIG);
    } else {
      print("Button reset press.");
    }

    print("Restarting...");

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

  String ssid = "Module - ";
  ssid += _type;
  ssid += " (";
  ssid += deviceName[0] != '\0' ? deviceName : String(random(0xffff), HEX);
  ssid += ")";

  WiFi.setHostname("iotz.local");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid.c_str(), _apPassword.c_str());

  print("SSID: ");
  print(ssid);
  print("PASS: ");
  print(_apPassword);
  print("Local server address: ");
  print(WiFi.softAPIP());
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
    print("ERROR on loading \"index.html\" file");
  }

  if (fileSuccess) {
    _htmlSuccess = fileSuccess.readString();
  } else {
    print("ERROR on loading \"success.html\" file");
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

    print("Device name: "+deviceName);
    print("Password: "+password);
    print("Server address: "+serverIp+":"+serverPort);

    Config.setDeviceName(deviceName);
    Config.setNetworkSsid(ssid);
    Config.setNetworkPassword(password);
    Config.setServerIp(serverIp);
    Config.setServerPort(serverPort);
    Config.setDeviceMode(Modes.SLAVE);

    _webServer.send(200, "text/html", _parseHTML(_htmlSuccess));

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    print("Restarting...");

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

  print("Try to connect to: ");
  print(ssid);
  print("With password: ");
  print(password);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    #ifdef MODULE_CAN_DEBUG
      Serial.print(".");
    #endif

    vTaskDelay(500 / portTICK_PERIOD_MS);
  }

  print("");
  print("Connected to: ");
  print(Config.getNetworkSsid());
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
  print("Add listener: "+topic);

  _events[topic] = listener;
}

void ModuleCore::_onConnected() {
  StaticJsonDocument<PACKET_SIZE> doc;
  JsonObject data = doc.to<JsonObject>();

  data["name"]     = Config.getDeviceName();
  data["type"]     = _type;
  data["version"]  = _version;
  Serial.println("onConnected");

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
    print("Topic not found: "+topic);
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
    Serial.println(pin);
    Serial.println(level);

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