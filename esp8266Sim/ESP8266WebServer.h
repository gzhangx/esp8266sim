#include "ESP8266WiFi.h"
#include <map>
#include <vector>
typedef void(*web_fun_cb)(void);

class ESP8266WebServer {
    gCTcpIp tcp;
    int _port;
    int _method;    
    std::map<std::string, web_fun_cb> _map;    
    std::map<std::string, std::string> argMap;
    std::vector<std::string> argNames;
    std::vector<std::string> argVals;
public:
    ESP8266WebServer(int port);
    void on(const char* name, web_fun_cb func);
    void onNotFound(web_fun_cb func);
    void begin();
    int method();
    void send(int code, const char* contentType, String content);
    inline const char * uri() { return ""; }
    inline String arg(int who ) { 
        if (who >= argVals.size()) return "";
        return String(argVals[who]);
    }
    inline String arg(String who) { return argMap[who]; }
    inline size_t args() { return argNames.size(); }
    String argName(int who);

    void handleClient();
};

const int HTTP_POST = 2;
const int HTTP_GET = 1;