/**
 * Socket.h
 * @author: Renan Vaz <renan.c.vaz@gmail.com>
 */

#ifndef Socket_h
#define Socket_h

#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <functional>

// PACKET BUFFER SIZE
#define PACKET_SIZE 512

class Socket
{
  public:
    Socket();

    void connect(IPAddress ip, uint16_t port);
    void connect();
    void disconnect();
    void onMessage(std::function<void(const JsonObject &message)> cb);

    void loop();
    void send(const JsonObject &message);
  private:
    IPAddress _ip;
    uint16_t _port;

    std::function<void(const JsonObject &message)> _onMessageCb;

    // uint8_t* _packetBuffer[PACKET_SIZE];
    uint8_t* _packetBuffer = 0;
};

#endif
