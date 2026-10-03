#pragma once

#include <Arduino.h>

struct RtcDateTime {
    uint16_t year = 0;
    uint8_t month = 0;
    uint8_t day = 0;
    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;
    uint8_t week = 0;
    RtcDateTime() = default;
    RtcDateTime(uint16_t y, uint8_t mo, uint8_t d, uint8_t h, uint8_t mi, uint8_t s, uint8_t w = 0)
        : year(y), month(mo), day(d), hour(h), minute(mi), second(s), week(w) {}
};

class RtcSensor {
public:
    bool begin();
    bool update();
    bool setDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
    bool ready() const { return ready_; }
    const RtcDateTime &dateTime() const { return dateTime_; }

private:
    RtcDateTime dateTime_{};
    bool ready_ = false;
};

extern RtcSensor rtcSensor;
