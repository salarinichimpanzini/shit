#include "board_io.h"
#include "config.h"

BoardIO boardIO;

bool BoardIO::writeReg(uint8_t reg, const uint8_t *data, size_t len) {
    if (!wire_ || !data || !len) return false;
    wire_->beginTransmission(C5Config::IO_ADDR);
    wire_->write(reg);
    wire_->write(data, len);
    return wire_->endTransmission() == 0;
}

bool BoardIO::write8(uint8_t reg, uint8_t value) {
    return writeReg(reg, &value, 1);
}

bool BoardIO::readReg(uint8_t reg, uint8_t *data, size_t len) {
    if (!wire_ || !data || !len) return false;
    wire_->beginTransmission(C5Config::IO_ADDR);
    wire_->write(reg);
    if (wire_->endTransmission(false) != 0) return false;
    if (wire_->requestFrom(C5Config::IO_ADDR, static_cast<uint8_t>(len)) != len) return false;
    for (size_t i = 0; i < len; ++i) data[i] = wire_->read();
    return true;
}

bool BoardIO::begin(TwoWire &wire) {
    wire_ = &wire;

    // The integrated LCD/touch example uses the 8-bit CH32V003 register bank.
    // Keep the factory-controlled reset/power outputs enabled.
    wire_->begin(C5Config::I2C_SDA, C5Config::I2C_SCL);
    wire_->setClock(C5Config::I2C_FREQ_HZ);

    ready_ = write8(C5Config::IO_MODE_REG, 0xFF) &&
             write8(C5Config::IO_OUTPUT_REG, 0x00) &&
             write8(C5Config::IO_PWM_REG, 0x00) &&
             write8(C5Config::IO_MODE_REG,
                    static_cast<uint8_t>((1U << C5Config::IO_RESET_TOUCH) |
                                         (1U << C5Config::IO_RESET_LCD) |
                                         (1U << C5Config::IO_POWER)));
    if (!ready_) return false;

    outputBits_ = static_cast<uint8_t>((1U << C5Config::IO_RESET_TOUCH) |
                                       (1U << C5Config::IO_RESET_LCD));
    if (!write8(C5Config::IO_OUTPUT_REG, outputBits_)) {
        ready_ = false;
        return false;
    }
    delay(50);

    outputBits_ = 0;
    if (!write8(C5Config::IO_OUTPUT_REG, outputBits_)) {
        ready_ = false;
        return false;
    }
    delay(50);

    outputBits_ = static_cast<uint8_t>((1U << C5Config::IO_RESET_TOUCH) |
                                       (1U << C5Config::IO_RESET_LCD) |
                                       (1U << C5Config::IO_POWER));
    ready_ = write8(C5Config::IO_OUTPUT_REG, outputBits_);
    return ready_;
}

bool BoardIO::setOutput(uint8_t pin, bool level) {
    if (!ready_ || pin > 7) return false;
    if (pin == C5Config::IO_RESET_TOUCH || pin == C5Config::IO_RESET_LCD || pin == C5Config::IO_POWER) {
        return false;
    }
    if (level) outputBits_ |= static_cast<uint8_t>(1U << pin);
    else outputBits_ &= static_cast<uint8_t>(~(1U << pin));
    return write8(C5Config::IO_OUTPUT_REG, outputBits_);
}

bool BoardIO::readInputs(uint16_t &bits) {
    uint8_t data[1] = {0};
    if (!readReg(C5Config::IO_OUTPUT_REG + 1, data, 1)) return false;
    bits = data[0];
    return true;
}

uint16_t BoardIO::readAdc() {
    uint8_t data[2] = {0, 0};
    if (!readReg(0x06, data, 2)) return 0;
    return static_cast<uint16_t>(data[0]) | static_cast<uint16_t>(data[1] << 8);
}

bool BoardIO::setBacklight(uint8_t percent) {
    if (!ready_) return false;
    if (percent > 100) percent = 100;
    const uint8_t pwm = static_cast<uint8_t>((static_cast<uint16_t>(percent) * 255U) / 100U);
    return write8(C5Config::IO_PWM_REG, pwm);
}

bool BoardIO::resetLcd() {
    if (!ready_) return false;
    if (!setOutput(C5Config::IO_RESET_LCD, true)) {
        // Reserved pins can still be used internally for reset sequences.
        outputBits_ |= static_cast<uint8_t>(1U << C5Config::IO_RESET_LCD);
        if (!write8(C5Config::IO_OUTPUT_REG, outputBits_)) return false;
    }
    delay(10);
    outputBits_ &= static_cast<uint8_t>(~(1U << C5Config::IO_RESET_LCD));
    if (!write8(C5Config::IO_OUTPUT_REG, outputBits_)) return false;
    delay(20);
    outputBits_ |= static_cast<uint8_t>(1U << C5Config::IO_RESET_LCD);
    if (!write8(C5Config::IO_OUTPUT_REG, outputBits_)) return false;
    delay(120);
    return true;
}

bool BoardIO::resetTouch() {
    if (!ready_) return false;
    outputBits_ |= static_cast<uint8_t>(1U << C5Config::IO_RESET_TOUCH);
    if (!write8(C5Config::IO_OUTPUT_REG, outputBits_)) return false;
    delay(10);
    outputBits_ &= static_cast<uint8_t>(~(1U << C5Config::IO_RESET_TOUCH));
    if (!write8(C5Config::IO_OUTPUT_REG, outputBits_)) return false;
    delay(20);
    outputBits_ |= static_cast<uint8_t>(1U << C5Config::IO_RESET_TOUCH);
    if (!write8(C5Config::IO_OUTPUT_REG, outputBits_)) return false;
    delay(120);
    return true;
}
