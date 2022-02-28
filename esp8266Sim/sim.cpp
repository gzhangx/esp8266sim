#include "stdafx.h"

//#include <ESP8266WiFi.h>
//#include <gglocal.h>
//#define SendKey 0  //Button to send data Flash BTN on NodeMCU

#ifndef STASSID
#define STASSID1 "test"
#define STAPSK1  "test"

#endif

//Server connect to WiFi Network
const char* ssid = STASSID;
const char* password = STAPSK;

const bool SERIAL_DEBUG = false;
int port = 8888;  //Port number
WiFiServer server(port);
int count = 0;

CheapStepper cstepper(D5, D6, D7, D8);

//=======================================================================
//                    Loop
//=======================================================================

WiFiClient outClient;
const int BUFSIZE = 1024;
const int CMDSIZE = 64;

typedef enum {
    SND_INIT,
    SND_RSP,
    SND_BODY,
    SND_DONE,
} SndState;

struct SendInfo {
    char buf[BUFSIZE];
    SndState state = SND_INIT;
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
void print(const char * fmt, ...) {
    va_list args;
    char buf[1000];
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Serial.print(buf);
}

void fillRegisterCmd(char * buf, const char *ip) {
    snprintf(buf, BUFSIZE, "GET /esp/register?mac=%s&ip=%s  HTTP/1.0\r\n\r\n", WiFi.macAddress().c_str(), ip);
}
void parseResponse(WiFiClient& client, SendInfo* info);


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

void runMotor(MotorCmd & cmd) {
    if (cmd.amount <= 0) return;
    if (cmd.rpm) {
        cstepper.setRpm(cmd.rpm);
    }
    int amount = cmd.amount;
    while (amount--) {
        cstepper.move(cmd.dir, 1);
    }
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
        }
        else if (!strcmp(cmd, "enabled")) {
            int enabled = atoi(val);
            print("resolved=%s=%i\n", cmd, enabled);
            mcmd.enabled = (bool)(enabled!=0);
        }
    }

    runMotor(mcmd);
}

void fillSendInfo(SendInfo & inf, const char * fmt, ...) {
    va_list args;    
    va_start(args, fmt);
    vsnprintf(inf.buf, sizeof(inf.buf), fmt, args);
    va_end(args);    
    inf.state = SND_INIT;
}
bool checkAction(SendInfo * info) {
    if (info->state == SND_DONE) return false;
    if (!outClient.connected() && info->state == SND_INIT) {
        outClient.connect("192.168.1.41", 8101);
        outClient.write((uint8_t*)info->buf, strlen(info->buf));
        info->state = SND_RSP;
        info->lastActionTime = millis();
        info->rsp[0] = 0;
        info->inBody = false;
        info->curPos = 0;
    }    
    
    if (!outClient.connected()) return false;

    //printf("debugremove trying to read\n");
    if (info->needParseRsp) {
        parseResponse(outClient, info);
        return true;
    }
    else {
        info->state = SND_DONE;
        outClient.stop();
    }
    return false;
}

void parseResponse(WiFiClient& client, SendInfo* info) {
    if (client.available()) {
        while (client.available()) {
            char c = static_cast<char>(client.read());
            info->buf[info->curPos++] = c;
            info->buf[info->curPos] = 0;
            if (c == '\n') {
                info->buf[info->curPos] = 0;
                //Serial.print(info->buf);
                if (info->curPos == 2) {
                    info->state = SND_BODY;
                }
                info->buf[0] = 0;
                info->curPos = 0;
            }
            delay(0);
        }
    }

    if (info->buf[0] != 0) {
        //Serial.println(info->buf);
        strcpy(info->rsp, info->buf);
        if (info->state == SND_BODY) {
            if (SERIAL_DEBUG) Serial.println("parse done");
            info->state = SND_DONE;
            parseSendInfo(*info);            
            client.stop();
        }
    }
}

SendInfo sndState;

long lastCheckTime = millis();



//=======================================================================
//                    Power on setup
//=======================================================================
void setup()
{
    delay(500);
    cstepper.setRpm(12);
    Serial.begin(115200);
    Serial.setDebugOutput(true);
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

    delay(1000);
    char buf[512];
    sprintf(buf, "%i.%i.%i.%i", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
    fillSendInfo(sndState, "GET /esp/register?mac=%s&ip=%s  HTTP/1.0\r\n\r\n", WiFi.macAddress().c_str(), buf);
    checkAction(&sndState);
}



void loop()
{        
    cstepper.move(1, 1);
    //sndState.needParseRsp = true;
    WiFiClient client = server.available();

    const long curMills = millis();
    const long timeDiff = curMills - lastCheckTime;
    //print("%ld %ld %ld\n", curMills, lastCheckTime, timeDiff);
    if (millis() - lastCheckTime > 10000) {
        if (SERIAL_DEBUG) print("%ld %ld %ld\n", curMills, lastCheckTime, timeDiff);
        lastCheckTime = curMills ;
        fillSendInfo(sndState, "GET /esp/getAction?mac=%s  HTTP/1.0\r\n\r\n", WiFi.macAddress().c_str());       
    }
    // if (checkAction(&sndState)) {
    // loopReceivedCommands(sndState);
    //}
    if (client) {
        if (client.connected())
        {
            if (SERIAL_DEBUG) Serial.println("Client Connected");
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
        SendInfo srvInf;
        MotorCmd mcmd;
        parseResponse(client, &srvInf);
        loopReceivedCommands(srvInf, mcmd);
        if (SERIAL_DEBUG) Serial.println("Client disconnected");
    }
    else {
       delay(500);
    }

    
}