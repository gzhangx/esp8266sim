
#ifndef CHEAPSTEAPPERH
#define CHEAPSTEAPPERH
const int D4 = 4;
const int D5 = 5;
const int D6 = 6;
const int D7 = 7;
const int D8 = 8;

class CheapStepper {
public:
    CheapStepper(int a, int b, int c, int d);
    void  setRpm(int rpm);
    void move(int a, int b);
    inline int getPin(int p) {  return 0; // default 0
    }
};

#endif