#pragma once

#include <Arduino.h>
#define LV_CONF_INCLUDE_SIMPLE
#include <lvgl.h>
#include "config.h"
#include <esp_lcd_panel_io.h>

class C5Display {
public:
    bool begin();
    bool beginBus();
    bool beginPanel();
    void tick(uint32_t elapsedMs);
    bool ready() const { return ready_; }
    lv_disp_t *display() const { return display_; }
    bool setRotation(uint8_t rotation);
    uint8_t rotation() const { return rotation_; }
    uint16_t width() const { return currentWidth_; }
    uint16_t height() const { return currentHeight_; }

private:
    static constexpr uint16_t WIDTH = 320;
    static constexpr uint16_t HEIGHT = 480;
    static constexpr size_t BUFFER_PIXELS = static_cast<size_t>(HEIGHT) * C5Config::LCD_BUFFER_LINES;

    void *lcdIo_ = nullptr;
    lv_disp_draw_buf_t drawBuf_{};
    lv_color_t *drawBuffer1_ = nullptr;
    lv_color_t *drawBuffer2_ = nullptr;
    lv_disp_drv_t displayDriver_{};
    lv_disp_t *display_ = nullptr;
    bool ready_ = false;
    uint8_t rotation_ = 0;
    uint16_t currentWidth_ = WIDTH;
    uint16_t currentHeight_ = HEIGHT;
    volatile bool transferDone_ = false;

    bool initBus();
    bool initPanel();
    bool sendCommand(uint8_t command, const uint8_t *data = nullptr, size_t length = 0);
    bool sendArea(int x1, int y1, int x2, int y2, const void *pixels, size_t bytes);

    static bool colorDone(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *eventData, void *userCtx);
    static void flushCb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *colorP);
    static C5Display *active_;
    void applyRotationCommand(uint8_t rotation);
};

extern C5Display display;
