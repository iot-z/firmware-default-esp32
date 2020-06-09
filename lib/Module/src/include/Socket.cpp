/**
 * Socket.cpp
 * @author: Renan Vaz <renan.c.vaz@gmail.com>
 */

#include "Socket.h"

WiFiClient client;

Socket::Socket()
{

}

void Socket::onMessage(std::function<void(const JsonObject &message)> cb) {
  _onMessageCb = cb;
}

void Socket::connect(IPAddress ip, uint16_t port)
{
  _ip = ip;
  _port = port;

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

    delay(500);
  }

  #ifdef MODULE_CAN_DEBUG
  Serial.println("Socket connected!");
  #endif
}

void Socket::disconnect()
{
  client.stop();
}

void Socket::loop()
{
  if (client.connected()) {
    size_t _packetSize = client.available();

    if (_packetSize) {
      client.read(_packetBuffer, _packetSize);
      _packetBuffer[_packetSize] = '\0';

      StaticJsonDocument<PACKET_SIZE> doc;

      DeserializationError error = deserializeJson(doc, _packetBuffer);

      if (error) {
        #ifdef MODULE_CAN_DEBUG
          Serial.println("Error on parsing payload:");
          Serial.println(_packetBuffer);
        #endif
      } else {
        JsonObject message = doc.to<JsonObject>();

        _onMessageCb(message);
      }
    }
  } else {
    connect();
  }
}

void Socket::send(const JsonObject &message)
{
  serializeJson(message, client);

  #ifdef MODULE_CAN_DEBUG
    Serial.print("Send message: ");
    Serial.println(message);
  #endif
}
