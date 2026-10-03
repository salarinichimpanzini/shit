#pragma once

#include <Arduino.h>
#define LV_CONF_INCLUDE_SIMPLE
#include <lvgl.h>

class FT6336Touch {
public:
    bool begin();
    bool bindLvgl();
    bool read(uint16_t *x, uint16_t *y, uint8_t *count, uint8_t maxPoints = 1);
    bool ready() const { return ready_; }
    lv_indev_t *lvglIndev() const { return indev_; }

private:
    bool ready_ = false;
    lv_indev_t *indev_ = nullptr;
    lv_indev_drv_t indevDrv_{};

    bool readRegs(uint8_t reg, uint8_t *data, size_t len);
    void mapPoint(uint16_t rawX, uint16_t rawY, lv_coord_t &x, lv_coord_t &y) const;
    static void lvglReadCb(lv_indev_drv_t *drv, lv_indev_data_t *data);
};

extern FT6336Touch touch;
