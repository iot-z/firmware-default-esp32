#include "Module.h"

String ID      = "031d8494-9d53-4f2c-bd4c-72e5fc5b3080";
String TYPE    = "Module Default";
String VERSION = "1.0.0";

void setup() {
  #ifdef MODULE_CAN_DEBUG
    // Init Serial for log data
    Serial.begin(9600);
    delay(42);
  #endif

  // Module.setResetButtonPin(D1);
  // Module.setLedStatusPin(LED_BUILTIN);

  Module.setup(ID, TYPE, VERSION);

  Module.createDefaultAPI();

  // Create your API here
}

void loop() {

}
