#pragma once

#include <Arduino.h>

class Shtc3Sensor {
public:
    bool begin();
    bool update();
    bool ready() const { return ready_; }
    float temperatureC() const { return temperatureC_; }
    float humidityPct() const { return humidityPct_; }
    unsigned int deviceId() const { return deviceId_; }

private:
    bool ready_ = false;
    float temperatureC_ = 0.0f;
    float humidityPct_ = 0.0f;
    uint16_t deviceId_ = 0;

    static uint8_t crc(const uint8_t *data, size_t len);
    bool command(uint16_t cmd);
    bool wake();
    void sleep();
    bool readWordWithCrc(uint16_t &value);
};

extern Shtc3Sensor shtc3;
