// esp8266Sim.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ESP8266WiFi.h"


int main()
{
    gCTcpIp ip;
    bool ok = ip.tcp_client("www.google.com", 80);
    printf("connect=%i\n", ok);
    ok = ip.sendstr("GET / HTTP/1.0\r\n\r\n");
    printf("send=%i\n", ok);
    
    WiFiClient cc(ip);
    while (cc.available()) {
        printf("%c", cc.read());
        break;
    }    


    setup();

    while (true) {
        loop();
    }
}




