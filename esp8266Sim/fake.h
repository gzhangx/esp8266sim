#include <string>

#ifndef ESPFAKETOTAL
#define ESPFAKETOTAL
class SerialClass {
public:
    void write(char c);
    void println(const char *s);
    void println(int num);
    void print(const char * fmt, ...);
    inline void begin(int num) {}
    inline int available() {
        return 0;
    }
    inline int read() { return 0; }
    inline void setDebugOutput(bool b) {}
};

extern SerialClass Serial;


void delay(int ms);


const int INPUT_PULLUP = 1;
void pinMode(int key, int mode);

void setup();
void loop();

long millis();

void digitalWrite(int pin, int mode);

void configTime(long gmtOffset_sec, int daylightOffset_sec, const char*ntpServer);

const int LED_BUILTIN = 4;
const int OUTPUT = 1;

class String : public std::string {
public:
    String(const char * str);
    String(std::string str);
    String operator=(const char* msg);
    String operator+(String msg);
};


#endif