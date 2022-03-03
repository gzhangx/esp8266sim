#include "stdafx.h"
#include <ESP8266WebServer.h>


ESP8266WebServer::ESP8266WebServer(int port) {
    _port = port;
}

DWORD WINAPI myThread(LPVOID lpParameter)
{
    ESP8266WebServer* srv = (ESP8266WebServer*)lpParameter;
    
    return 0;
}
void ESP8266WebServer::on(const char* name, web_fun_cb func) {
    _map[name] = func;
}

void ESP8266WebServer::onNotFound(web_fun_cb func){}

void ESP8266WebServer::begin() {
    tcp.tcp_server(_port, 5, false);

    //DWORD myThreadID;
    //HANDLE myHandle = CreateThread(0, 0, myThread, this, 0, &myThreadID);

}
int ESP8266WebServer::method() { return _method; }
void ESP8266WebServer::send(int code, const char* contentType, String content) {


}

String ESP8266WebServer::argName(int who) { 
    return argNames[who]; 
}

void ESP8266WebServer::handleClient() {
    gCTcpIp ip = tcp.accept_connection();
    WiFiClient cli = WiFiClient(ip);
    if (!cli.connected()) return;
    char buf[1024];
    int i = 0;
    bool firstLine = true;
    char cmd[512];
    cmd[0] = 0;
    int dataLen = 0;
    while (cli.available()) {
        int c = cli.read();
        if (c <= 0) break;
        buf[i++] = c;
        dataLen = i;
        buf[i] = 0;
        if (c == '\n') {
            buf[i] = 0;
            buf[i - 1] = 0;
            buf[i - 2] = 0;            
            i = 0;
            if (firstLine) {
                firstLine = false;
                int start = 0;
                for (i = 0; i < sizeof(buf); i++) {
                    if (buf[i] == ' ' || !buf[i]) {
                        buf[i] = 0;
                        start = i + 1;
                        if (!strcmp(buf, "GET")) {
                            _method = HTTP_GET;
                        }
                        else if (!strcmp(buf, "POST")) {
                            _method = HTTP_POST;
                        }
                        break;                        
                    }
                }
                for (i = start; i < sizeof(buf); i++) {
                    if (buf[i] == ' ' || !buf[i]) {
                        buf[i] = 0;
                        strcpy(cmd, buf + start);
                        break;
                    }
                }
            }
            else {
                if (i == 2) {
                    printf("end found");
                }
            }
        }
    }

    bool hasCurItem = false;
    bool nameState = true;
    int curPos = 0;
    

    std::string name;
    std::string val;
    argNames.clear();
    argVals.clear();
    argMap.clear();
    bool noVal = false;
    for (int i = 0; i < dataLen; i++) {
        const char c = buf[i];
        if (nameState) {
            if (c == '=' || c == '&' || c == 0) {                
                if (c != '&') nameState = false;
                else {
                    argVals.push_back("");
                }
                buf[i] = 0;
                name = buf + curPos;
                argNames.push_back(name);                
                argMap[name] = "";
                curPos = i + 1;                
                if (c == 0) {
                    break;
                }
                continue;
            }
        }
        else {
            if (c == '&' || c == 0) {                
                nameState = true;
                buf[i] = 0;
                val = buf + curPos;
                curPos = i + 1;                
                argVals.push_back(val);
                argMap[name] = val;
                if (c == 0) {

                    break;
                }                
            }
        }
    }

    if (curPos < dataLen) {
        val = buf + curPos;
        curPos = i + 1;
        argVals.push_back(val);
        argMap[name] = val;
    }
    if (_map.find(cmd) != _map.end()) {
        _map[cmd]();
    }
}