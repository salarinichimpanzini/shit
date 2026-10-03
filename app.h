#pragma once

#include <Arduino.h>

class C5Application {
public:
    bool begin();
    void loop();

private:
    void updateSensors();
    void updateRtc();
    void updatePower();
    void updateIo();
    void logToSd();
    void processSerial();

    bool initialized_ = false;
    unsigned long lastSensorMs_ = 0;
    unsigned long lastRtcMs_ = 0;
    unsigned long lastPowerMs_ = 0;
    unsigned long lastIoMs_ = 0;
    unsigned long lastUiMs_ = 0;
    unsigned long lastSdLogMs_ = 0;
    unsigned long lastPmuIrqMs_ = 0;
    unsigned long lastLvglMs_ = 0;
    unsigned long lastWifiMs_ = 0;
    unsigned long lastTimeMs_ = 0;
};

extern C5Application app;
