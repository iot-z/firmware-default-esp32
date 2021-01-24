#include "Module.h"

String ID      = "031d8494-9d53-4f2c-bd4c-72e5fc5b3080";
String TYPE    = "Module Default";
String VERSION = "1.0.0";

void setup()
{
  #ifdef MODULE_CAN_DEBUG
    // Init Serial for log data
    Serial.begin(115200);
    while (!Serial) continue;
  #endif

  // Module.setResetButtonPin(D1);
  // Module.setLedStatusPin(LED_BUILTIN);
  // Module.setApPassword("123456789");

  Module.setup(ID, TYPE, VERSION);

  Module.createArduinoApi();

  // Create your API here

  Module.on("abc", [&](const JsonObject &in, const JsonObject &out) {
    serializeJsonPretty(in, Serial);
    Serial.println("");
  });
}

void loop()
{
  // int status = digitalRead(34);

  // if (status != currentStatus) {
  //   time_t now = millis();
  //   Serial.print(status ? "ON" : "OFF");
  //   Serial.print(" ");
  //   Serial.println(now - oldtime);
  //   currentStatus = status;
  //   oldtime = now;
  //   delay(9);
  // }
}
