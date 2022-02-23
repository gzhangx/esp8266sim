#pragma once
#include "libs\gtcpip.h"


const int RDBUFMAX = 512;
class WiFiClient {
    gCTcpIp tcp;
    char buf[RDBUFMAX];
    int curPos = 0;
    int curLen = 0;
public:
    WiFiClient(){
        curPos = 0;
        curLen = 0;
    }
    WiFiClient(gCTcpIp & t) {
        tcp = t;
        curPos = 0;
        curLen = 0;
    }
    bool connected() {
        return tcp.connected();
    }

    bool available() {
        if (curLen <= 0 || curPos >= curLen) {
            curLen = tcp.recv(buf, RDBUFMAX);
            curPos = 0;
        }
        return curPos < curLen;
    }
    char read() {        
        return buf[curPos++];
    }
};


class WiFiServer {
    gCTcpIp tcp;
    int _port;
public:
    WiFiServer(int port) {
        _port = port;
    }
    void begin() {
        tcp.tcp_server(_port, 5, false);
    }

    WiFiClient available() {
        gCTcpIp ip = tcp.accept_connection();
        return WiFiClient(ip);
    }


};

const int WL_CONNECTED = 1;
const int WIFI_STA = 1111;
class WiFiClass {
public:
    void mode(int md){}
    void begin(const char * ssid, const char * password) {}
    int status() {
        return WL_CONNECTED;
    }
    char * localIP() {
        return "test";
    }
};

WiFiClass WiFi;

class SerialClass {
public:
    void println() {
        printf("");
    }
    void println(const char *s) {
        printf("%s\n",s);
    }
    void println(int num) {
        printf("%i\n",num);
    }
    void print(const char * fmt, ...) {
        va_list args;
        char buf[1000];
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        printf(buf);
    }
    void begin(int num) {}
};

SerialClass Serial;


void delay(int ms) {
    Sleep(ms);
}