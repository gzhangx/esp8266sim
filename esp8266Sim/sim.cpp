#include "stdafx.h"

//#define SendKey 0  //Button to send data Flash BTN on NodeMCU

int port = 8888;  //Port number
WiFiServer server(port);

//Server connect to WiFi Network
const char *ssid = "---------";  //Enter your wifi SSID
const char *password = "--------";  //Enter your wifi Password

int count = 0;

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
};

void fillRegisterCmd(char * buf, const char *ip) {
    snprintf(buf, BUFSIZE, "GET /esp/register?mac=%s&ip=%s  HTTP/1.0\r\n\r\n", WiFi.macAddress().c_str(), ip);
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
        if (outClient.available()) {
            while (outClient.available()) {
                char c = static_cast<char>(outClient.read());
                info->buf[info->curPos++] = c;
                info->buf[info->curPos] = 0;
                if (c == '\n') {
                    info->buf[info->curPos] = 0;
                    Serial.print(info->buf);
                    info->buf[0] = 0;
                    info->curPos = 0;
                    if (info->curPos == 2) {
                        info->state = SND_BODY;
                    }
                }
                delay(0);
            }
        }
        {
            if (info->buf[0] != 0) {
                Serial.println(info->buf);
                if (info->state == SND_BODY) {
                    Serial.println("parse done");
                    info->state = SND_DONE;
                    outClient.stop();
                }
            }
        }
    }
    else {
        info->state = SND_DONE;
        outClient.stop();
    }
}

SendInfo sndState;

long lastCheckTime = millis();
bool registered = false;
//=======================================================================
//                    Power on setup
//=======================================================================
void setup()
{
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

void loop()
{        
    //sndState.needParseRsp = true;
    WiFiClient client = server.available();

    if (!registered) {
        fillRegisterCmd(sndState.buf, "testip");
        registered = true;
    }
    Serial.print("%l %l %i\n", millis(), lastCheckTime, millis() - lastCheckTime);
    if (millis() - lastCheckTime > 1000) {
        Serial.print("%l %l %i\n", millis(), lastCheckTime, millis() - lastCheckTime);
        lastCheckTime = millis();
        snprintf(sndState.buf, BUFSIZE, "GET /esp/getAction?mac=%s  HTTP/1.0\r\n\r\n", WiFi.macAddress().c_str());
        sndState.state = SND_INIT;
    }
    checkAction(&sndState);
    if (client) {
        if (client.connected())
        {
            Serial.println("Client Connected");
        }

        while (client.connected()) {
            while (client.available()>0) {
                // read data from the connected client
                Serial.write(client.read());
            }
            //Send Data to connected client
            while (Serial.available()>0)
            {
                client.write(Serial.read());
            }
        }
        client.stop();
        Serial.println("Client disconnected");
    }
    else {
       delay(500);
    }

    
}