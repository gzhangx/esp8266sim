#pragma once
#include "libs\gtcpip.h"
#include "fake.h"
#include "IPAddress.h"

const int RDBUFMAX = 512;
class WiFiClient {
    gCTcpIp tcp;
    char buf[RDBUFMAX];
    int curPos = 0;
    int curLen = 0;
public:
    WiFiClient();
    WiFiClient(gCTcpIp & t);
    virtual operator bool();
    bool connected();

    bool connect(const char * host, int port);

    int available();
    int read();
    virtual size_t write(const uint8_t *buf, size_t size);
    size_t write(char c);
    void stop();
};


class WiFiServer {
    gCTcpIp tcp;
    int _port;
public:
    WiFiServer(int port);
    void begin();

    WiFiClient available();
};

const int WL_CONNECTED = 1;
const int WIFI_STA = 1111;
class WiFiClass {
public:
    inline void mode(int md){}
    inline void begin(const char * ssid, const char * password) {}
    inline int status() {
        return WL_CONNECTED;
    }
    inline char * localIP() {
        return "test";
    }
    inline std::string macAddress() {
        return "testmac";
    }
    inline char * softAPIP() {
        return "testapip";
    }
    inline void softAP(const char *addr) {}
    inline void softAPdisconnect() {}
    inline void disconnect() {}
};

extern WiFiClass WiFi;
