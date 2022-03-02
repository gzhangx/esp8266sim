#include "stdafx.h"
SerialClass Serial;


String::String(const char * str) : std::string(str) {
}
String String::operator=(const char* msg) {
    return String(msg);
}

String String::operator+(String msg) {
    return *this + msg;
}