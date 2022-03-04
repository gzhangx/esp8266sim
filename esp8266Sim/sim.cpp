#include "stdafx.h"
#include "CheapStepper.h"
#include <ESP8266WebServer.h>
//#include <ESP8266WiFi.h>
//#include <gglocal.h>
//#define SendKey 0  //Button to send data Flash BTN on NodeMCU
#include "time.h"
#ifndef STASSID
#define STASSID "test"
#define STAPSK  "test"

#endif

//Server connect to WiFi Network
const char* ssid = STASSID;
const char* password = STAPSK;

const bool SERIAL_DEBUG = false;
int port = 8888;  //Port number
WiFiServer server(port);
int count = 0;

CheapStepper cstepper(D5, D6, D7, D8);

const int led = LED_BUILTIN;
//=======================================================================
//                    Loop
//=======================================================================

WiFiClient outClient;
const int BUFSIZE = 1024;
const int CMDSIZE = 64;

ESP8266WebServer webserver(80);

struct SendInfo {
    char buf[BUFSIZE];
    bool needParseRsp = true;
    long lastActionTime = 0;
    bool inBody = false;
    int curPos = 0;
    char rsp[BUFSIZE];    
    int cmdPos[CMDSIZE];
};


struct MotorCmd {
    int rpm = 0;
    int dir;
    int amount;
    bool enabled;
};

char print_buf[1000];
void print(const char * fmt, ...) {
    va_list args;    
    va_start(args, fmt);
    vsnprintf(print_buf, sizeof(print_buf), fmt, args);
    va_end(args);
    Serial.print(print_buf);
}

void stopMotor() {
    for (int i = 0; i < 4; i++) {
        digitalWrite(cstepper.getPin(i), 0);
    }
}


void syncTime() {
    static long lastSyncTime = millis();
    static long curSyncTime = 0;
    curSyncTime = millis();
    if (curSyncTime - lastSyncTime < 20000) return;
    lastSyncTime = curSyncTime;
    Serial.println("sync time");
    const char* ntpServer = "pool.ntp.org";
    const long  gmtOffset_sec = 18000;   //Replace with your GMT offset (seconds)
    const int   daylightOffset_sec = 0; //Replace with your daylight offset (seconds)
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}
void fillRegisterCmd(char * buf, const char *ip) {
    snprintf(buf, BUFSIZE, "GET /esp/register?mac=%s&ip=%s  HTTP/1.0\r\n\r\n", WiFi.macAddress().c_str(), ip);
}
bool parseResponse(WiFiClient& client, SendInfo* info);


void parseSendInfo(SendInfo & info) {
    int who = 0;
    bool hasCurItem = false;
    bool nameState = true;
    int curPos = 0;
    info.cmdPos[who++] = 0;
    info.cmdPos[1] = -1;
    for (int i = 0; i < sizeof(info.rsp); i++) {
        const char c = info.rsp[i];
        if (nameState) {
            if (c == '=' || c=='&' || c==0) {
                info.cmdPos[who] = i + 1;
                if (c != '&') nameState = false;                                
                info.rsp[i] = 0;
                if (c != '=') {
                    info.cmdPos[who++] = i;
                    info.cmdPos[who] = i + 1;
                }
                who++;
                if (c == 0) {
                    info.cmdPos[who++] = -1;
                    info.cmdPos[who] = -1;
                    break;
                }
                continue;
            }
        }
        else {
            if (c == '&' || c == 0) {
                if (c == 0) {
                    info.rsp[i] = 0;
                    info.cmdPos[who++] = -1;
                    break;
                }
                nameState = true;
                info.cmdPos[who++] = i + 1;
                info.rsp[i] = 0;
                continue;
            }
        }
    }
}


void debugTest() {
    SendInfo info;
    strcpy(info.rsp, "a=b&cccc=dddd&name=gang&test=2&onlyfirst&onlyeq=&a1&a2");
    parseSendInfo(info);
    for (int i = 0; i < sizeof(info.cmdPos); i+=2) {
        int p1 = info.cmdPos[i];
        int p2 = info.cmdPos[i+1];
        if (p1 < 0) break;
        Serial.println(info.rsp + p1);
        Serial.println(info.rsp + p2);
        Serial.println("");
    }
}

void runMotor(int dir, int amount) {    
    while (amount>0) {
        cstepper.move(dir, 1);
        delay(0);
        amount--;
    }
}

void runMotor(MotorCmd & cmd) {    
    runMotor(cmd.dir, cmd.amount);    
    cmd.amount = 0;
}

void loopReceivedCommands(SendInfo & info, MotorCmd & mcmd) {
    mcmd.rpm = 0;
    mcmd.amount = 0;
    mcmd.dir = 0;
    for (int i = 0; i < sizeof(info.cmdPos); i += 2) {
        int p1 = info.cmdPos[i];
        int p2 = info.cmdPos[i + 1];
        if (p1 < 0 || p2 < 0) break;
        char * cmd = info.rsp + p1;
        char * val = info.rsp + p2;
        if (SERIAL_DEBUG) print("'%s': '%s'\n", cmd, val);
        if (!strcmp(cmd, "rpm")) {
            int rpm = atoi(val);
            if (rpm <= 0) rpm = 1;
            print("resolved=%s=%i\n", cmd, rpm);
            mcmd.rpm = rpm;
            cstepper.setRpm(rpm);
        }
        else if (!strcmp(cmd, "dir")) {
            int dir = atoi(val);
            print("resolved=%s=%i\n", cmd, dir);
            mcmd.dir = dir;
        }
        else if (!strcmp(cmd, "amount")) {
            int amount = atoi(val);
            print("resolved=%s=%i\n", cmd, amount);
            mcmd.amount = amount;
            runMotor(mcmd);
        }
        else if (!strcmp(cmd, "enabled")) {
            int enabled = atoi(val);
            print("resolved=%s=%i\n", cmd, enabled);
            mcmd.enabled = (bool)(enabled!=0);
            if (!mcmd.enabled) stopMotor();
        }
    }
    
}

void fillSendInfo(SendInfo & inf, const char * fmt, ...) {
    va_list args;    
    va_start(args, fmt);
    vsnprintf(inf.buf, sizeof(inf.buf), fmt, args);
    va_end(args);    
}
bool checkAction(SendInfo * info) {

    outClient.connect("192.168.1.41", 8101);
    if (SERIAL_DEBUG) Serial.print(info->buf);
    outClient.write((uint8_t*)info->buf, strlen(info->buf));
    info->lastActionTime = millis();
    info->rsp[0] = 0;
    info->inBody = false;
    info->curPos = 0;


    if (!outClient.connected()) {
        if (SERIAL_DEBUG) Serial.println("!!!!! check action ignored, not connected");
        return false;
    }

    //printf("debugremove trying to read\n");

    return parseResponse(outClient, info);
}

bool parseResponse(WiFiClient& client, SendInfo* info) {
    for (int i = 0; i < 100; i++) {
        if (client.available()) break;
        delay(10);
    }
    info->buf[0] = 0;
    bool hasBody = false;
    if (client.available()) {
        while (client.available()) {
            char c = static_cast<char>(client.read());
            info->buf[info->curPos++] = c;
            info->buf[info->curPos] = 0;            
            if (c == '\n') {                
                if (SERIAL_DEBUG) Serial.println(info->buf);
                //Serial.print(info->buf);
                if (info->curPos == 2) {
                    hasBody = true;
                }
                info->buf[0] = 0;
                info->curPos = 0;
            }
            delay(0);
        }
    }
    client.stop();

    if (hasBody) {
        if (SERIAL_DEBUG) Serial.println(info->buf);
        strcpy(info->rsp, info->buf);
        if (SERIAL_DEBUG) Serial.println("parse done");
        parseSendInfo(*info);
        return true;
    }
    return false;
}

SendInfo sndState;

long lastCheckTime = millis();

const String postForms = "<html>\
  <head>\
    <title>ESP8266 Web Server POST handling</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>POST form data to / motorCommand /</h1><br>\
    <form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/motorCommand/\">\
      RPM <input type=\"text\" name=\"rpm\" value=\"1\"><br>\
DIR <input type=\"text\" name=\"dir\" value=\"0\"><br>\
Amount <input type=\"text\" name=\"amount\" value=\"100\"><br>\
Enabled <input type=\"text\" name=\"enabled\" value=\"0\"><br>\
AP <input type=\"text\" name=\"ap\" value=\"0\"><br>\
ST <input type=\"text\" name=\"st\" value=\"0\"><br>\
      <input type=\"submit\" value=\"Submit\">\
    </form>\
  </body>\
</html>";
void handleRoot() {
    digitalWrite(led, 1);
    webserver.send(200, "text/html", postForms);
    digitalWrite(led, 0);
}

void handlePlain() {
    if (webserver.method() != HTTP_POST) {
        digitalWrite(led, 1);
        webserver.send(405, "text/plain", "Method Not Allowed");
        digitalWrite(led, 0);
    }
    else {
        digitalWrite(led, 1);
        webserver.send(200, "text/plain", String("POST body was:\n") + webserver.arg("plain"));
        digitalWrite(led, 0);
    }
}

void handleForm() {
    if (webserver.method() != HTTP_POST) {
        digitalWrite(led, 1);
        webserver.send(405, "text/plain", "Method Not Allowed");
        digitalWrite(led, 0);
    }
    else {
        digitalWrite(led, 1);
        const int MAXRPL = 128;
        char buf[MAXRPL];
        buf[0] = 0;
        int rpm = 1;
        int dir = 1;
        int amount = 0;
        for (uint8_t i = 0; i < webserver.args(); i++) {
            String cmd = webserver.argName(i);
            String val = webserver.arg(i);
            if (cmd ==  "rpm") {
                rpm = atoi(val.c_str());
                if (rpm <= 0) rpm = 1;
                print("resolved=%s=%i\n", cmd.c_str(), rpm);
                cstepper.setRpm(rpm);
                strncat(buf, "set rpm=", MAXRPL);
                strncat(buf, val.c_str(), MAXRPL);
                strncat(buf, " ", MAXRPL);
            }
                else if (cmd == "dir") {
                    dir = atoi(val.c_str());
                    print("resolved=%s=%i\n", cmd.c_str(), dir);
                    strncat(buf, "set dir=", MAXRPL);
                    strncat(buf, val.c_str(), MAXRPL);
                    strncat(buf, " ", MAXRPL);
                }
                else if (cmd == "amount") {
                    amount = atoi(val.c_str());                    
                    if (amount > 0) {
                        if (amount > 10000) {                            
                            amount = 10000;  
                            print("hard limiting amount %s to %i\n", val, amount);
                        }
                        runMotor(dir, amount);
                        strncat(buf, "set amount=", MAXRPL);
                        strncat(buf, val.c_str(), MAXRPL);
                        strncat(buf, " ", MAXRPL);
                    }
                    print("resolved=%s=%i\n", cmd.c_str(), amount);
                }
                else if (cmd == "enabled") {
                    int enabled = val == "1";
                    print("resolved=%s=%i\n", cmd.c_str(), enabled);                    
                    if (!enabled) stopMotor();
                    strncat(buf, "set enabled=", MAXRPL);
                    strncat(buf, val.c_str(), MAXRPL);
                    strncat(buf, " ", MAXRPL);
                }
                else if (cmd == "ap" && val == "1") {
                    Serial.print("going ap");
                    WiFi.softAP("esp_8266");
                    Serial.print("going ap addr");
                    Serial.println(WiFi.softAPIP());
                }
                else if (cmd == "st" && val == "1") {
                    WiFi.softAPdisconnect();
                    WiFi.disconnect();
                    WiFi.mode(WIFI_STA);
                    WiFi.begin(ssid, password); //Connect to wifi
                }
        }
        webserver.send(200, "text/plain", "OK");
        digitalWrite(led, 0);
    }
}

void handleNotFound() {
    digitalWrite(led, 1);
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += webserver.uri();
    message += "\nMethod: ";
    message += (webserver.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += webserver.args();
    message += "\n";
    for (uint8_t i = 0; i < webserver.args(); i++) {
        message += " " + webserver.argName(i) + ": " + webserver.arg(i) + "\n";
    }
    webserver.send(404, "text/plain", message);
    digitalWrite(led, 0);
}

//=======================================================================
//                    Power on setup
//=======================================================================
void setup()
{
    pinMode(led, OUTPUT);
    digitalWrite(led, 1);
    delay(500);
    digitalWrite(led, 0);
    cstepper.setRpm(12);
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    //pinMode(SendKey, INPUT_PULLUP);  //Btn to send data
    Serial.println("");

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

    delay(1000);
    char buf[512];
    sprintf(buf, "%i.%i.%i.%i", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
    fillSendInfo(sndState, "GET /esp/register?mac=%s&ip=%s  HTTP/1.0\r\n\r\n", WiFi.macAddress().c_str(), buf);
    checkAction(&sndState);


    webserver.on("/", handleRoot);    
    webserver.on("/motorCommand/", handleForm);
    webserver.onNotFound(handleNotFound);
    webserver.begin();
}

SendInfo srvInf;
MotorCmd mcmd;

void loop()
{        
    syncTime();
    webserver.handleClient();
    //cstepper.move(1, 1);
    //sndState.needParseRsp = true;
    WiFiClient client = server.available();

    const long curMills = millis();
    const long timeDiff = curMills - lastCheckTime;
    //print("%ld %ld %ld\n", curMills, lastCheckTime, timeDiff);
    if (millis() - lastCheckTime > 10000) {
        print("%ld %ld %ld\n", curMills, lastCheckTime, timeDiff);
        lastCheckTime = curMills ;
        fillSendInfo(sndState, "GET /esp/getAction?mac=%s  HTTP/1.0\r\n\r\n", WiFi.macAddress().c_str());       
        digitalWrite(led, 1);
        if (checkAction(&sndState)) {
         loopReceivedCommands(sndState, mcmd);
        }
        digitalWrite(led, 0);
    }
    
    if (client) {
        if (client.connected())
        {
            if (SERIAL_DEBUG) Serial.println("Client Connected");            
            digitalWrite(led, 1);
            if (parseResponse(client, &srvInf)) {
                loopReceivedCommands(srvInf, mcmd);
            }
            digitalWrite(led, 0);
            if (SERIAL_DEBUG) Serial.println("Client disconnected");
        }

        //while (client.connected()) {
        //    while (client.available()>0) {
        //        Serial.write(client.read());
        //    }
        //    while (Serial.available()>0)
        //    {
        //        client.write(Serial.read());
        //    }
        //}
        //client.stop();        
    }
    else {
       delay(1);
    }

    
}