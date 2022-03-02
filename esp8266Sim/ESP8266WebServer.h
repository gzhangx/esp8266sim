#include "ESP8266WiFi.h"
typedef void(*web_fun_cb)(void);

class ESP8266WebServer {
public:
    ESP8266WebServer(int port);
    void on(const char* name, web_fun_cb func);
    void onNotFound(web_fun_cb func);
    void begin();
    int method();
    void send(int code, const char* contentType, String content);
    inline const char * uri() { return ""; }
    inline String arg(int who ) { return ""; }
    inline String arg(String who) { return ""; }
    inline int args() { return 0; }
    inline String argName(int who) { return ""; }
};

const int HTTP_POST = 0;
const int HTTP_GET = 0;