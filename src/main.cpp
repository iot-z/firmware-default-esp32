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

  int read[] = {1, 2, 3};
} CI_CD4051;

int STATE = 0;
int STATE_LAST = 0;

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
  STATE = read();

  if (STATE != STATE_LAST) {
    StaticJsonDocument<PACKET_SIZE> doc;
    JsonObject data = doc.to<JsonObject>();

    Module.send('change', data);
  }

  int status = digitalRead(34);

  if (status != currentStatus) {
    time_t now = millis();
    Serial.print(status ? "ON" : "OFF");
    Serial.print(" ");
    Serial.println(now - oldtime);
    currentStatus = status;
    oldtime = now;
    delay(9);
  }
}

int read() {
  int state = 0;

  int b0;
  int b1;
  int b2;

  int value;

  int i, iLen, k, kLen;

  for (i = 0, iLen = 2; i <= iLen; i++) {
    for (k = 0, kLen = 7; k <= kLen; i++) {
      r0 = bitRead(k, 0);
      r1 = bitRead(k, 1);
      r2 = bitRead(k, 2);

      digitalWrite(s0, r0);
      digitalWrite(s1, r1);
      digitalWrite(s2, r2);

      value = digitalRead(CI_CD4051.read[i]);

      bitwrite(state, k + (8 * i), value);
    }
  }

  Serial.println(state, BIN);

  return state;
}

void pulse(int state) {
  byte off = 0b00000000;

  digitalWrite(latchPin, LOW);
  shiftOut(CI_74HC595.data, CI_74HC595.clockPin, LSBFIRST, (state >> (8*0)) & 0xff);
  shiftOut(CI_74HC595.data, CI_74HC595.clockPin, LSBFIRST, (state >> (8*1)) & 0xff);
  shiftOut(CI_74HC595.data, CI_74HC595.clockPin, LSBFIRST, (state >> (8*2)) & 0xff);
  digitalWrite(latchPin, HIGH);

  delay(1000);

  digitalWrite(latchPin, LOW);
  shiftOut(CI_74HC595.data, CI_74HC595.clockPin, LSBFIRST, off);
  shiftOut(CI_74HC595.data, CI_74HC595.clockPin, LSBFIRST, off);
  shiftOut(CI_74HC595.data, CI_74HC595.clockPin, LSBFIRST, off);
  digitalWrite(latchPin, HIGH);
}