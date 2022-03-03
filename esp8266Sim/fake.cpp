#include "stdafx.h"
SerialClass Serial;


String::String(const char * str) : std::string(str) {
}

String::String(std::string str) : std::string(str) {
}

String String::operator=(const char* msg) {
    return String(msg);
}


String String::operator+(String msg) {
    return (std::string(msg) + std::string(*this)).c_str();
}




long millis() {
    return clock();
}

void digitalWrite(int pin, int mode) {}

void configTime(long gmtOffset_sec, int daylightOffset_sec, const char*ntpServer) {}