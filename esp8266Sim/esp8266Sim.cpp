// esp8266Sim.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


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
    return 0;
}

