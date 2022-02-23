// esp8266Sim.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ESP8266WiFi.h"

void setup();
int main()
{

    gCTcpIp ip;
    bool ok = ip.tcp_client("www.google.com", 80);
    printf("connect=%i\n", ok);
    ok = ip.sendstr("GET / HTTP/1.0\r\n\r\n");
    printf("send=%i\n", ok);
    char buf[512];
    int len = ip.recv(buf, 511);    
    buf[len] = 0;
    printf("%s\n", buf);
    printf("got len %i\n", len);

    setup();
    return 0;
}


WiFiServer server(80);

const char * ssid = "test";
const char * password = "test";
int port = 80;
void setup() {
    Serial.begin(115200);
    //pinMode(SendKey, INPUT_PULLUP);  //Btn to send data
    Serial.println();

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password); //Connect to wifi

                                // Wait for connection  
    Serial.println("Connecting to Wifi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        delay(500);
    }

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);

    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    server.begin();
    Serial.print("Open Telnet and connect to IP:");
    Serial.print(WiFi.localIP());
    Serial.print(" on port ");
    Serial.println(port);
}
