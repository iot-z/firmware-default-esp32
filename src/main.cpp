#include "Module.h"

String ID      = "031d8494-9d53-4f2c-bd4c-72e5fc5b3080";
String TYPE    = "Module Default";
String VERSION = "1.0.0";

void loopIotz(void *p)
{
  for(;;) {
    Module.loop();
  }
}

void setup()
{
  #ifdef MODULE_CAN_DEBUG
    // Init Serial for log data
    Serial.begin(9600);
    delay(42);
  #endif

  // Module.setResetButtonPin(D1);
  // Module.setLedStatusPin(LED_BUILTIN);

  Module.setup(ID, TYPE, VERSION);

  Module.createArduinoApi();

  // Create your API here

  xTaskCreatePinnedToCore(loopIotz, "loopIotz", 10000, NULL, 1, NULL, 0);
}

void loop()
{

}


