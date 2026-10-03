#pragma once

#include <Arduino.h>

class PowerPmu {
public:
    bool begin();
    bool update();
    bool ready() const { return ready_; }
    bool batteryConnected() const { return batteryConnected_; }
    bool charging() const { return charging_; }
    bool discharging() const { return discharging_; }
    uint8_t batteryPercent() const { return batteryPercent_; }
    uint16_t batteryMv() const { return batteryMv_; }
    uint16_t vbusMv() const { return vbusMv_; }
    uint16_t systemMv() const { return systemMv_; }
    float temperatureC() const { return temperatureC_; }
    void pollIrqEvents();
    void printRails();

private:
    bool ready_ = false;
    bool batteryConnected_ = false;
    bool charging_ = false;
    bool discharging_ = false;
    uint8_t batteryPercent_ = 0;
    uint16_t batteryMv_ = 0;
    uint16_t vbusMv_ = 0;
    uint16_t systemMv_ = 0;
    float temperatureC_ = 0.0f;
};

extern PowerPmu pmu;
