#pragma once

#include <Arduino.h>
#include <Wire.h>

class BoardIO {
public:
    bool begin(TwoWire &wire = Wire);
    bool setOutput(uint8_t pin, bool level);
    bool readInputs(uint16_t &bits);
    uint16_t readAdc();
    bool setBacklight(uint8_t percent);
    bool resetLcd();
    bool resetTouch();
    bool isReady() const { return ready_; }
    uint8_t outputBits() const { return outputBits_; }

private:
    TwoWire *wire_ = &Wire;
    uint8_t outputBits_ = 0;
    bool ready_ = false;

    bool writeReg(uint8_t reg, const uint8_t *data, size_t len);
    bool write8(uint8_t reg, uint8_t value);
    bool readReg(uint8_t reg, uint8_t *data, size_t len);
};

extern BoardIO boardIO;
