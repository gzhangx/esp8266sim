#include "stdafx.h"

 WiFiClass WiFi;
 SerialClass Serial;

void pinMode(int key, int mode) {}
void delay(int ms) {
    Sleep(ms);
}



WiFiClient::WiFiClient() {
        curPos = 0;
        curLen = 0;
    }
WiFiClient::WiFiClient(gCTcpIp & t) {
        tcp = t;
        curPos = 0;
        curLen = 0;
    }
    WiFiClient::operator bool() {
        return tcp.valid();
    }
    bool WiFiClient::connected() {
        return tcp.connected();
    }

    int WiFiClient::available() {
        if (curLen <= 0 || curPos >= curLen) {
            curLen = tcp.recv(buf, RDBUFMAX);
            curPos = 0;
        }
        return curLen - curPos; // curPos < curLen;
    }
    char WiFiClient::read() {
        return buf[curPos++];
    }
    size_t WiFiClient::write(char c) {
        return tcp.sendbyte(c);
    }
    void WiFiClient::stop() {
        tcp.close();
    }




    WiFiServer::WiFiServer(int port) {
        _port = port;
    }
    void WiFiServer::begin() {
        tcp.tcp_server(_port, 5, false);
    }

    WiFiClient WiFiServer::available() {
        gCTcpIp ip = tcp.accept_connection();
        return WiFiClient(ip);
    }




    void SerialClass::println() {
        printf("");
    }
    void SerialClass::write(char c) {
        printf("%c", c);
    }
    void SerialClass::println(const char *s) {
        printf("%s\n", s);
    }
    void SerialClass::println(int num) {
        printf("%i\n", num);
    }
    void SerialClass::print(const char * fmt, ...) {
        va_list args;
        char buf[1000];
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        printf(buf);
    }
  