#include "Module.h"

String ID      = "031d8494-9d53-4f2c-bd4c-72e5fc5b3080";
String TYPE    = "Module Default";
String VERSION = "1.0.0";


struct CI_74HC595_Struct
{
  int latch = 8; // ST_CP
  int clock = 12; // SH_CP
  int data = 11; // DS
} CI_74HC595;

struct CI_CD4051_Struct
{
  int s0 = 2;
  int s1 = 3;
  int s2 = 4;

  int read[3] = {1, 2, 3};
} CI_CD4051;

int STATE = 0;
int STATE_LAST = 0;


int readMux()
{
  int state = 0;

  int b0;
  int b1;
  int b2;

  int value;

  int i, iLen, k, kLen;

  for (i = 0, iLen = 2; i <= iLen; i++) {
    for (k = 0, kLen = 7; k <= kLen; i++) {
      b0 = bitRead(k, 0);
      b1 = bitRead(k, 1);
      b2 = bitRead(k, 2);

      digitalWrite(CI_CD4051.s0, b0);
      digitalWrite(CI_CD4051.s1, b1);
      digitalWrite(CI_CD4051.s2, b2);

      value = digitalRead(CI_CD4051.read[i]);

      bitWrite(state, k + (8 * i), value);
    }
  }

  Serial.println(state, BIN);

  return state;
}

void pulse(int state)
{
  byte off = 0b00000000;

  digitalWrite(CI_74HC595.latch, LOW);
  shiftOut(CI_74HC595.data, CI_74HC595.clock, LSBFIRST, (state >> (8*0)) & 0xff);
  shiftOut(CI_74HC595.data, CI_74HC595.clock, LSBFIRST, (state >> (8*1)) & 0xff);
  shiftOut(CI_74HC595.data, CI_74HC595.clock, LSBFIRST, (state >> (8*2)) & 0xff);
  digitalWrite(CI_74HC595.latch, HIGH);

  delay(1000);

  digitalWrite(CI_74HC595.latch, LOW);
  shiftOut(CI_74HC595.data, CI_74HC595.clock, LSBFIRST, off);
  shiftOut(CI_74HC595.data, CI_74HC595.clock, LSBFIRST, off);
  shiftOut(CI_74HC595.data, CI_74HC595.clock, LSBFIRST, off);
  digitalWrite(CI_74HC595.latch, HIGH);
}

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

  Module.on("pulse", [&](const JsonObject &in, const JsonObject &out) {
    serializeJsonPretty(in, Serial);
    Serial.println("");

    pulse(in["state"]);
  });

  pinMode(CI_74HC595.latch, OUTPUT);
  pinMode(CI_74HC595.clock, OUTPUT);
  pinMode(CI_74HC595.data, OUTPUT);

  pinMode(CI_CD4051.s0, OUTPUT);
  pinMode(CI_CD4051.s1, OUTPUT);
  pinMode(CI_CD4051.s2, OUTPUT);

  pinMode(CI_CD4051.read[0], INPUT);
  pinMode(CI_CD4051.read[1], INPUT);
  pinMode(CI_CD4051.read[2], INPUT);
}

void loop()
{
  STATE = readMux();

  if (STATE != STATE_LAST) {
    StaticJsonDocument<PACKET_SIZE> doc;
    JsonObject data = doc.to<JsonObject>();

    Module.send("change", data);
  }

  delay(9);
}
