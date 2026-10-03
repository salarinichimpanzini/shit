#include "touch_ft6336.h"
#include "config.h"
#include "board_io.h"
#include "settings.h"
#include "display.h"
#include <Wire.h>

FT6336Touch touch;

bool FT6336Touch::readRegs(uint8_t reg, uint8_t *data, size_t len) {
    Wire.beginTransmission(C5Config::TOUCH_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    const size_t got = Wire.requestFrom(C5Config::TOUCH_ADDR, static_cast<uint8_t>(len));
    if (got != len) return false;
    for (size_t i = 0; i < len; ++i) data[i] = static_cast<uint8_t>(Wire.read());
    return true;
}

bool FT6336Touch::begin() {
    pinMode(C5Config::TOUCH_INT, INPUT);
    boardIO.resetTouch();
    delay(20);

    // Read the FT6336 chip/vendor IDs when available. Failure here is not fatal;
    // TD_STATUS probing below remains the authoritative runtime check.
    uint8_t chip = 0, vendor = 0;
    readRegs(0xA3, &chip, 1);
    readRegs(0xA8, &vendor, 1);

    Wire.beginTransmission(C5Config::TOUCH_ADDR);
    ready_ = (Wire.endTransmission() == 0);
    if (ready_) {
        Serial.printf("FT6336 touch ready (chip 0x%02X vendor 0x%02X)\n", chip, vendor);
    } else {
        Serial.println("FT6336 touch not found at 0x38");
    }
    return ready_;
}

void FT6336Touch::mapPoint(uint16_t rawX, uint16_t rawY, lv_coord_t &x, lv_coord_t &y) const {
    int32_t tx = rawX;
    int32_t ty = rawY;
    if (C5Config::TOUCH_SWAP_XY) { const int32_t t = tx; tx = ty; ty = t; }
    if (C5Config::TOUCH_INVERT_X) tx = static_cast<int32_t>(C5Config::TOUCH_RAW_WIDTH - 1U) - tx;
    if (C5Config::TOUCH_INVERT_Y) ty = static_cast<int32_t>(C5Config::TOUCH_RAW_HEIGHT - 1U) - ty;

    const uint8_t r = display.rotation() & 0x03;
    const int32_t lw = display.width();
    const int32_t lh = display.height();
    switch (r) {
        case 0:
            x = static_cast<lv_coord_t>(constrain(tx, 0L, lw - 1));
            y = static_cast<lv_coord_t>(constrain(ty, 0L, lh - 1));
            break;
        case 1:
            x = static_cast<lv_coord_t>(constrain(ty, 0L, lw - 1));
            y = static_cast<lv_coord_t>(constrain((C5Config::TOUCH_RAW_WIDTH - 1) - tx, 0L, lh - 1));
            break;
        case 2:
            x = static_cast<lv_coord_t>(constrain((C5Config::TOUCH_RAW_WIDTH - 1) - tx, 0L, lw - 1));
            y = static_cast<lv_coord_t>(constrain((C5Config::TOUCH_RAW_HEIGHT - 1) - ty, 0L, lh - 1));
            break;
        default:
            x = static_cast<lv_coord_t>(constrain((C5Config::TOUCH_RAW_HEIGHT - 1) - ty, 0L, lw - 1));
            y = static_cast<lv_coord_t>(constrain(tx, 0L, lh - 1));
            break;
    }
}

bool FT6336Touch::read(uint16_t *x, uint16_t *y, uint8_t *count, uint8_t maxPoints) {
    if (!x || !y || !count || !maxPoints || !ready_) return false;

    uint8_t points = 0;
    if (!readRegs(0x02, &points, 1)) {
        *count = 0;
        return false;
    }

    points &= 0x0F;
    if (points > 2) points = 2;
    if (points > maxPoints) points = maxPoints;
    if (!points) {
        *count = 0;
        return true;
    }

    uint8_t data[12] = {};
    if (!readRegs(0x03, data, static_cast<size_t>(6 * points))) {
        *count = 0;
        return false;
    }

    for (uint8_t i = 0; i < points; ++i) {
        const uint16_t rawX = static_cast<uint16_t>(((data[i * 6] & 0x0F) << 8) | data[i * 6 + 1]);
        const uint16_t rawY = static_cast<uint16_t>(((data[i * 6 + 2] & 0x0F) << 8) | data[i * 6 + 3]);

        // Do not reject the raw FT6336 range. This panel reports 480x320 and is
        // rotated relative to the 320x480 LCD; mapPoint() performs the conversion.
        lv_coord_t mappedX = 0, mappedY = 0;
        mapPoint(rawX, rawY, mappedX, mappedY);
        x[i] = static_cast<uint16_t>(mappedX);
        y[i] = static_cast<uint16_t>(mappedY);
    }

    *count = points;
    return true;
}

void FT6336Touch::lvglReadCb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    auto *self = static_cast<FT6336Touch *>(drv ? drv->user_data : nullptr);
    if (!self || !data) return;

    static lv_coord_t lastX = C5Config::LCD_WIDTH / 2;
    static lv_coord_t lastY = C5Config::LCD_HEIGHT / 2;

    uint16_t x = 0, y = 0;
    uint8_t count = 0;
    const bool ok = self->read(&x, &y, &count, 1);

    if (ok && count) {
        lastX = static_cast<lv_coord_t>(x);
        lastY = static_cast<lv_coord_t>(y);
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }

    data->point.x = lastX;
    data->point.y = lastY;
    data->continue_reading = false;
}

bool FT6336Touch::bindLvgl() {
    if (!ready_ || indev_) return ready_ && indev_;

    lv_indev_drv_init(&indevDrv_);
    indevDrv_.type = LV_INDEV_TYPE_POINTER;
    indevDrv_.read_cb = lvglReadCb;
    indevDrv_.user_data = this;
    indev_ = lv_indev_drv_register(&indevDrv_);

    Serial.println(indev_ ? "FT6336 -> LVGL input registered" : "FT6336 -> LVGL input registration FAILED");
    return indev_ != nullptr;
}
