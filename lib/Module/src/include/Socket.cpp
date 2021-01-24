/**
 * Socket.cpp
 * @author: Renan Vaz <renan.c.vaz@gmail.com>
 */

#include <StreamUtils.h>
#include "Socket.h"

WiFiClient client;

Socket::Socket()
{

}

void Socket::onConnected(std::function<void()> cb) {
  _onConnectedCb = cb;
}

void Socket::onMessage(std::function<void(const JsonObject &message)> cb) {
  _onMessageCb = cb;
}

void Socket::connect(IPAddress ip, uint16_t port)
{
  _ip = ip;
  _port = port;

  client.setNoDelay(1);

  connect();
}

void Socket::connect()
{
  #ifdef MODULE_CAN_DEBUG
  Serial.println("Connecting socket...");
  #endif

  while (!client.connect(_ip, _port)) {
    #ifdef MODULE_CAN_DEBUG
    Serial.print(".");
    #endif

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  #ifdef MODULE_CAN_DEBUG
  Serial.println("Socket connected!");
  #endif

  _onConnectedCb();
}

void Socket::disconnect()
{
  client.stop();
}

void Socket::loop()
{
  if (client.connected()) {
    if (client.available()) {
      String payload = "";

      while (client.available() > 0) {
        char c = client.read();
        payload += c;
      }

      StaticJsonDocument<PACKET_SIZE> doc;

      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        #ifdef MODULE_CAN_DEBUG
          Serial.println("Error on parsing payload: "+payload);
        #endif
      } else {
        JsonObject message = doc.as<JsonObject>();

        #ifdef MODULE_CAN_DEBUG
          Serial.print("Received: ");
          serializeJson(doc, Serial);
          Serial.println("");
        #endif

        _onMessageCb(message);
      }
    }
  } else {
    connect();
  }
}

void Socket::send(const JsonObject &message)
{
  WriteBufferingStream bufferedClient{client, PACKET_SIZE};
  serializeJson(message, bufferedClient);

  #ifdef MODULE_CAN_DEBUG
    Serial.print("Sending: ");
    serializeJson(message, Serial);
    Serial.println("");
  #endif
}
