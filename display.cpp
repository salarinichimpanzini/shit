#include "display.h"
#include "config.h"
#include "board_io.h"
#include "settings.h"
#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_io_interface.h>
#include <esp_heap_caps.h>

C5Display display;
C5Display *C5Display::active_ = nullptr;

bool C5Display::colorDone(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t *, void *userCtx) {
    auto *self = static_cast<C5Display *>(userCtx);
    if (!self) self = active_;
    if (self) self->transferDone_ = true;
    return false;
}

bool C5Display::sendCommand(uint8_t command, const uint8_t *data, size_t length) {
    if (!lcdIo_) return false;
    return esp_lcd_panel_io_tx_param(static_cast<esp_lcd_panel_io_handle_t>(lcdIo_), command, data, length) == ESP_OK;
}

bool C5Display::initBus() {
    // CS lines for the other SPI slaves must be inactive during SD startup.
    pinMode(C5Config::LCD_CS, OUTPUT);
    digitalWrite(C5Config::LCD_CS, HIGH);

    spi_bus_config_t busConfig = {};
    busConfig.sclk_io_num = C5Config::LCD_SCLK;
    busConfig.mosi_io_num = C5Config::LCD_MOSI;
    busConfig.miso_io_num = C5Config::LCD_MISO;
    busConfig.quadwp_io_num = -1;
    busConfig.quadhd_io_num = -1;
    const size_t maxDim = (C5Config::LCD_WIDTH > C5Config::LCD_HEIGHT) ? C5Config::LCD_WIDTH : C5Config::LCD_HEIGHT;
    busConfig.max_transfer_sz = static_cast<int>(maxDim * C5Config::LCD_BUFFER_LINES * sizeof(lv_color_t));

    const esp_err_t err = spi_bus_initialize(SPI2_HOST, &busConfig, SPI_DMA_CH_AUTO);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        Serial.printf("SPI2 init failed: %s\n", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool C5Display::initPanel() {
    esp_lcd_panel_io_spi_config_t ioConfig = {};
    ioConfig.dc_gpio_num = C5Config::LCD_DC;
    ioConfig.cs_gpio_num = C5Config::LCD_CS;
    ioConfig.pclk_hz = C5Config::LCD_SPI_HZ;
    ioConfig.spi_mode = 0;
    ioConfig.trans_queue_depth = 10;
    ioConfig.on_color_trans_done = colorDone;
    ioConfig.user_ctx = this;
    ioConfig.lcd_cmd_bits = 8;
    ioConfig.lcd_param_bits = 8;

    esp_lcd_panel_io_handle_t io = nullptr;
    esp_err_t err = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &ioConfig, &io);
    if (err != ESP_OK) {
        Serial.printf("LCD panel IO init failed: %s\n", esp_err_to_name(err));
        return false;
    }
    lcdIo_ = io;

    // ST7796 sequence from the board's working LVGL example.
    sendCommand(0x01);
    delay(20);
    sendCommand(0x11);
    delay(100);
    const uint8_t madctl = 0x48;
    const uint8_t colmod = 0x55;
    sendCommand(0x36, &madctl, 1);
    sendCommand(0x3A, &colmod, 1);
    const uint8_t f0c3[] = {0xC3}; sendCommand(0xF0, f0c3, sizeof(f0c3));
    const uint8_t f096[] = {0x96}; sendCommand(0xF0, f096, sizeof(f096));
    const uint8_t b4[] = {0x01}; sendCommand(0xB4, b4, sizeof(b4));
    const uint8_t b7[] = {0xC6}; sendCommand(0xB7, b7, sizeof(b7));
    const uint8_t e8[] = {0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33}; sendCommand(0xE8, e8, sizeof(e8));
    const uint8_t c1[] = {0x06}; sendCommand(0xC1, c1, sizeof(c1));
    const uint8_t c2[] = {0xA7}; sendCommand(0xC2, c2, sizeof(c2));
    const uint8_t c5[] = {0x18}; sendCommand(0xC5, c5, sizeof(c5));
    const uint8_t e0[] = {0xF0,0x09,0x0B,0x06,0x04,0x15,0x2F,0x54,0x42,0x3C,0x17,0x14,0x18,0x1B}; sendCommand(0xE0, e0, sizeof(e0));
    const uint8_t e1[] = {0xF0,0x09,0x0B,0x06,0x04,0x03,0x2D,0x43,0x42,0x3B,0x16,0x14,0x17,0x1B}; sendCommand(0xE1, e1, sizeof(e1));
    const uint8_t f03c[] = {0x3C}; sendCommand(0xF0, f03c, sizeof(f03c));
    const uint8_t f069[] = {0x69}; sendCommand(0xF0, f069, sizeof(f069));
    sendCommand(0x21);
    sendCommand(0x29);
    delay(120);
    return true;
}

bool C5Display::sendArea(int x1, int y1, int x2, int y2, const void *pixels, size_t bytes) {
    const uint8_t column[] = {
        static_cast<uint8_t>(x1 >> 8), static_cast<uint8_t>(x1),
        static_cast<uint8_t>(x2 >> 8), static_cast<uint8_t>(x2)
    };
    const uint8_t row[] = {
        static_cast<uint8_t>(y1 >> 8), static_cast<uint8_t>(y1),
        static_cast<uint8_t>(y2 >> 8), static_cast<uint8_t>(y2)
    };
    if (!sendCommand(0x2A, column, sizeof(column)) || !sendCommand(0x2B, row, sizeof(row))) return false;
    transferDone_ = false;
    if (esp_lcd_panel_io_tx_color(static_cast<esp_lcd_panel_io_handle_t>(lcdIo_), 0x2C, pixels, bytes) != ESP_OK) return false;
    const uint32_t start = millis();
    while (!transferDone_ && millis() - start < 1000) delay(1);
    return transferDone_;
}

void C5Display::flushCb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *colorP) {
    if (!active_) return;
    const size_t width = static_cast<size_t>(area->x2 - area->x1 + 1);
    const size_t height = static_cast<size_t>(area->y2 - area->y1 + 1);
    const size_t bytes = width * height * sizeof(lv_color_t);
    if (!active_->sendArea(area->x1, area->y1, area->x2, area->y2, colorP, bytes)) {
        Serial.println("LVGL LCD transfer failed");
    }
    lv_disp_flush_ready(drv);
}

bool C5Display::beginBus() {
    active_ = this;
    return boardIO.resetLcd() && initBus();
}

bool C5Display::beginPanel() {
    if (!active_) active_ = this;
    if (!initPanel()) return false;

    drawBuffer1_ = static_cast<lv_color_t *>(heap_caps_malloc(BUFFER_PIXELS * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA));
    drawBuffer2_ = static_cast<lv_color_t *>(heap_caps_malloc(BUFFER_PIXELS * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA));
    if (!drawBuffer1_ || !drawBuffer2_) {
        Serial.println("LVGL draw buffer allocation failed");
        return false;
    }

    lv_init();
    lv_disp_draw_buf_init(&drawBuf_, drawBuffer1_, drawBuffer2_, BUFFER_PIXELS);
    lv_disp_drv_init(&displayDriver_);
    displayDriver_.hor_res = WIDTH;
    displayDriver_.ver_res = HEIGHT;
    displayDriver_.flush_cb = flushCb;
    displayDriver_.draw_buf = &drawBuf_;
    display_ = lv_disp_drv_register(&displayDriver_);
    ready_ = display_ != nullptr;
    if (!ready_) return false;
    applyRotationCommand(settingsStore.rotation());
    return true;
}

void C5Display::applyRotationCommand(uint8_t rotation) {
    static const uint8_t madctl[4] = {0x48, 0x28, 0x88, 0xE8};
    rotation &= 0x03;
    sendCommand(0x36, &madctl[rotation], 1);
    rotation_ = rotation;
    if (rotation_ == 1 || rotation_ == 3) {
        currentWidth_ = HEIGHT;
        currentHeight_ = WIDTH;
    } else {
        currentWidth_ = WIDTH;
        currentHeight_ = HEIGHT;
    }
}

bool C5Display::setRotation(uint8_t rotation) {
    if (!ready_ || !display_) return false;
    applyRotationCommand(rotation);
    displayDriver_.hor_res = currentWidth_;
    displayDriver_.ver_res = currentHeight_;
    lv_disp_drv_update(display_, &displayDriver_);
    lv_obj_invalidate(lv_scr_act());
    lv_refr_now(display_);
    Serial.printf("LCD rotation %u -> %ux%u\n", rotation_ * 90U, currentWidth_, currentHeight_);
    return true;
}

bool C5Display::begin() {
    return beginBus() && beginPanel();
}

void C5Display::tick(uint32_t elapsedMs) {
    if (!ready_) return;
    lv_tick_inc(elapsedMs);
    lv_timer_handler();
}
