#include "sensor_rtc.h"

#include <Wire.h>
#include <time.h>

#include "config.h"

RtcSensor rtcSensor;

namespace {
constexpr uint8_t ADDR = C5Config::PCF85063_ADDR;
constexpr uint8_t SEC_REG = 0x04;
constexpr uint8_t CTRL1_REG = 0x00;
constexpr uint8_t CLOCK_STOP_BIT = 5;

uint8_t bcdToDec(uint8_t v) { return static_cast<uint8_t>((v >> 4) * 10 + (v & 0x0F)); }
uint8_t decToBcd(uint8_t v) { return static_cast<uint8_t>(((v / 10) << 4) | (v % 10)); }

bool writeRegs(uint8_t reg, const uint8_t *data, size_t len) {
    Wire.beginTransmission(ADDR);
    Wire.write(reg);
    for (size_t i = 0; i < len; ++i) Wire.write(data[i]);
    return Wire.endTransmission() == 0;
}

bool readRegs(uint8_t reg, uint8_t *data, size_t len) {
    Wire.beginTransmission(ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    const size_t got = Wire.requestFrom(ADDR, static_cast<uint8_t>(len));
    if (got != len) return false;
    for (size_t i = 0; i < len; ++i) data[i] = static_cast<uint8_t>(Wire.read());
    return true;
}

uint8_t weekday(uint16_t y, uint8_t m, uint8_t d) {
    struct tm t{};
    t.tm_year = y - 1900;
    t.tm_mon = m - 1;
    t.tm_mday = d;
    t.tm_isdst = -1;
    const time_t e = mktime(&t);
    if (e < 0) return 0;
    return static_cast<uint8_t>(t.tm_wday);
}

bool sane(uint16_t y, uint8_t mo, uint8_t d, uint8_t h, uint8_t mi, uint8_t s) {
    return y >= 2020 && y <= 2099 && mo >= 1 && mo <= 12 && d >= 1 && d <= 31 && h <= 23 && mi <= 59 && s <= 59;
}
}

bool RtcSensor::begin() {
    Wire.beginTransmission(ADDR);
    ready_ = Wire.endTransmission() == 0;
    if (!ready_) {
        Serial.println("PCF85063 not found at 0x51");
        return false;
    }

    // Force 24-hour mode and start the clock. This mirrors the working board example
    // without pulling the deprecated SensorPCF85063 wrapper into the build.
    uint8_t ctrl = 0;
    if (readRegs(CTRL1_REG, &ctrl, 1)) {
        ctrl &= static_cast<uint8_t>(~(1U << 1));
        ctrl &= static_cast<uint8_t>(~(1U << CLOCK_STOP_BIT));
        writeRegs(CTRL1_REG, &ctrl, 1);
    }

    const bool ok = update();
    Serial.println(ok ? "PCF85063 RTC ready" : "PCF85063 RTC read failed");
    return ok;
}

bool RtcSensor::update() {
    if (!ready_) return false;
    uint8_t b[7]{};
    if (!readRegs(SEC_REG, b, sizeof(b))) return false;

    const uint8_t secRaw = b[0];
    if (secRaw & 0x80) {
        Serial.println("PCF85063 clock integrity flag is set");
    }
    const uint8_t s = bcdToDec(secRaw & 0x7F);
    const uint8_t mi = bcdToDec(b[1] & 0x7F);
    const uint8_t h = bcdToDec(b[2] & 0x3F);
    const uint8_t d = bcdToDec(b[3] & 0x3F);
    const uint8_t w = bcdToDec(b[4] & 0x07);
    const uint8_t mo = bcdToDec(b[5] & 0x1F);
    const uint16_t y = static_cast<uint16_t>(2000 + bcdToDec(b[6]));
    if (!sane(y, mo, d, h, mi, s)) return false;
    dateTime_ = RtcDateTime(y, mo, d, h, mi, s, w);
    return true;
}

bool RtcSensor::setDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
    if (!ready_ || !sane(year, month, day, hour, minute, second)) return false;
    uint8_t b[7] = {
        static_cast<uint8_t>(decToBcd(second) & 0x7F),
        decToBcd(minute),
        decToBcd(hour),
        decToBcd(day),
        static_cast<uint8_t>(weekday(year, month, day) & 0x07),
        decToBcd(month),
        decToBcd(static_cast<uint8_t>(year % 100))
    };
    if (!writeRegs(SEC_REG, b, sizeof(b))) return false;
    return update();
}
