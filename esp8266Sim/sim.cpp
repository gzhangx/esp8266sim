#include "stdafx.h"

//#define SendKey 0  //Button to send data Flash BTN on NodeMCU



//Server connect to WiFi Network
const char *ssid = "---------";  //Enter your wifi SSID
const char *password = "--------";  //Enter your wifi Password

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


void loopReceivedCommands(SendInfo & info) {
    for (int i = 0; i < sizeof(info.cmdPos); i += 2) {
        int p1 = info.cmdPos[i];
        int p2 = info.cmdPos[i + 1];
        if (p1 < 0) break;
        Serial.println(info.rsp + p1);
        Serial.println(info.rsp + p2);
        Serial.println("");
    }
}

void fillSendInfo(SendInfo & inf, const char * fmt, ...) {
    va_list args;    
    va_start(args, fmt);
    vsnprintf(inf.buf, sizeof(inf.buf), fmt, args);
    va_end(args);    
    inf.state = SND_INIT;
}
void checkAction(SendInfo * info) {
    if (info->state == SND_DONE) return;
    if (!outClient.connected() && info->state == SND_INIT) {
        outClient.connect("192.168.1.41", 8101);
        outClient.write((uint8_t*)info->buf, strlen(info->buf));
        info->state = SND_RSP;
        info->lastActionTime = millis();
        info->rsp[0] = 0;
        info->inBody = false;
        info->curPos = 0;
    }    
    
    if (!outClient.connected()) return;

    //printf("debugremove trying to read\n");
    if (info->needParseRsp) {
        parseResponse(outClient, info);
    }
    else {
        info->state = SND_DONE;
        outClient.stop();
    }
}

void parseResponse(WiFiClient& client, SendInfo* info) {
    if (client.available()) {
        while (client.available()) {
            char c = static_cast<char>(client.read());
            info->buf[info->curPos++] = c;
            info->buf[info->curPos] = 0;
            if (c == '\n') {
                info->buf[info->curPos] = 0;
                Serial.print(info->buf);
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
        Serial.println(info->buf);
        strcpy(info->rsp, info->buf);
        if (info->state == SND_BODY) {
            Serial.println("parse done");
            info->state = SND_DONE;
            parseSendInfo(*info);
            loopReceivedCommands(*info);
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
    fillSendInfo(sndState, "GET /esp/register?mac=%s&ip=%s  HTTP/1.0\r\n\r\n", WiFi.macAddress().c_str(), WiFi.localIP());
    checkAction(&sndState);
}

void print(const char * fmt, ...) {
    va_list args;
    char buf[1000];
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Serial.print(buf);
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
        print("%l %l %i\n", curMills, lastCheckTime, timeDiff);
        lastCheckTime = curMills ;
        fillSendInfo(sndState, "GET /esp/getAction?mac=%s  HTTP/1.0\r\n\r\n", WiFi.macAddress().c_str());       
    }
    checkAction(&sndState);
    if (client) {
        if (client.connected())
        {
            Serial.println("Client Connected");
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
        parseResponse(client, &srvInf);
        loopReceivedCommands(srvInf);
        Serial.println("Client disconnected");
    }
    else {
       delay(500);
    }

    
}