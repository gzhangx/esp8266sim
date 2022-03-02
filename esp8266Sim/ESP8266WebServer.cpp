#include "stdafx.h"
#include <ESP8266WebServer.h>


ESP8266WebServer::ESP8266WebServer(int port) {

}


void ESP8266WebServer::on(const char* name, web_fun_cb func) {

}

void ESP8266WebServer::onNotFound(web_fun_cb func){}

void ESP8266WebServer::begin() {}
int ESP8266WebServer::method() { return 0; }
void ESP8266WebServer::send(int code, const char* contentType, String content) {

}