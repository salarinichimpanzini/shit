#include "sensor_shtc3.h"
#include "config.h"
#include <Arduino.h>
#include <Wire.h>

namespace {
constexpr uint16_t CMD_WAKE = 0x3517;
constexpr uint16_t CMD_SLEEP = 0xB098;
constexpr uint16_t CMD_SOFT_RESET = 0x805D;
constexpr uint16_t CMD_READ_ID = 0xEFC8;
constexpr uint16_t CMD_MEASURE_T_RH = 0x7866;
}

Shtc3Sensor shtc3;

uint8_t Shtc3Sensor::crc(const uint8_t *data, size_t len) {
    uint8_t crcValue = 0xFF;
    for (size_t i = 0; i < len; ++i) {
        crcValue ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crcValue = (crcValue & 0x80) ? static_cast<uint8_t>((crcValue << 1) ^ 0x31) : static_cast<uint8_t>(crcValue << 1);
        }
    }
    return crcValue;
}

bool Shtc3Sensor::command(uint16_t cmd) {
    Wire.beginTransmission(C5Config::SHTC3_ADDR);
    Wire.write(static_cast<uint8_t>(cmd >> 8));
    Wire.write(static_cast<uint8_t>(cmd));
    return Wire.endTransmission() == 0;
}

bool Shtc3Sensor::wake() {
    if (!command(CMD_WAKE)) return false;
    delayMicroseconds(240);
    return true;
}

void Shtc3Sensor::sleep() {
    command(CMD_SLEEP);
}

bool Shtc3Sensor::readWordWithCrc(uint16_t &value) {
    uint8_t buf[3] = {};
    if (Wire.requestFrom(C5Config::SHTC3_ADDR, static_cast<uint8_t>(3)) != 3) return false;
    for (uint8_t i = 0; i < 3; ++i) buf[i] = Wire.read();
    if (crc(buf, 2) != buf[2]) return false;
    value = static_cast<uint16_t>((buf[0] << 8) | buf[1]);
    return true;
}

bool Shtc3Sensor::begin() {
    if (!wake()) {
        Serial.println("SHTC3 not found at 0x70");
        return false;
    }
    command(CMD_SOFT_RESET);
    delay(2);

    if (!wake()) return false;
    const bool idOk = command(CMD_READ_ID) && readWordWithCrc(deviceId_);
    sleep();
    if (!idOk) {
        Serial.println("SHTC3 ID read failed");
        return false;
    }
    ready_ = true;
    Serial.printf("SHTC3 ready, ID 0x%04X\n", deviceId_);
    return true;
}

bool Shtc3Sensor::update() {
    if (!ready_) return false;
    uint8_t buf[6] = {};
    if (!wake()) return false;
    if (!command(CMD_MEASURE_T_RH)) {
        sleep();
        return false;
    }
    delay(20);
    const bool countOk = Wire.requestFrom(C5Config::SHTC3_ADDR, static_cast<uint8_t>(6)) == 6;
    if (!countOk) {
        sleep();
        return false;
    }
    for (uint8_t i = 0; i < 6; ++i) buf[i] = Wire.read();
    sleep();
    if (crc(&buf[0], 2) != buf[2] || crc(&buf[3], 2) != buf[5]) return false;

    const uint16_t rawTemp = static_cast<uint16_t>((buf[0] << 8) | buf[1]);
    const uint16_t rawHumidity = static_cast<uint16_t>((buf[3] << 8) | buf[4]);
    temperatureC_ = -45.0f + 175.0f * static_cast<float>(rawTemp) / 65535.0f;
    humidityPct_ = 100.0f * static_cast<float>(rawHumidity) / 65535.0f;
    return true;
}
