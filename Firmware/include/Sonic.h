#ifndef Sonic_h
#define Sonic_h

#include <Arduino.h>

class Sonic
{
public:
    Sonic(int trigPin, int echoPin)
    {
        this->trigPin = trigPin;
        this->echoPin = echoPin;
    }

    float getDistance()
    {
        float res = 0;
        long time = 0;

        digitalWrite(trigPin, LOW);
        delayMicroseconds(2);
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
        time = pulseIn(echoPin, HIGH);
        res = (time / 2) * soundSpeed;
        return res;
    }

private:
    const int soundSpeed = 0.0343; // [cm / microsecond]
    int trigPin;
    int echoPin;
};

#endif // Sonic_h