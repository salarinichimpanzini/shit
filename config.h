#pragma once

#include <Arduino.h>

namespace C5Config {
constexpr uint32_t SERIAL_BAUD = 115200;

// I2C shared by the board I/O expander, touch, SHTC3, QMI8658, PCF85063 and AXP2101.
constexpr int I2C_SDA = 27;
constexpr int I2C_SCL = 26;
constexpr uint32_t I2C_FREQ_HZ = 400000;

// ST7796 SPI bus.
constexpr int LCD_MISO = 2;
constexpr int LCD_MOSI = 7;
constexpr int LCD_SCLK = 6;
constexpr int LCD_CS = 8;
constexpr int LCD_DC = 5;
constexpr uint16_t LCD_WIDTH = 320;
constexpr uint16_t LCD_HEIGHT = 480;
constexpr uint32_t LCD_SPI_HZ = 60000000;

// IMPORTANT: the supplied board SD example initializes SPI2 with max_transfer_sz=4000.
// Keep the LVGL line buffer below the shared-DMA transfer ceiling; landscape width is 480 pixels.
constexpr size_t LCD_BUFFER_LINES = 4;
constexpr size_t LCD_MAX_TRANSFER_BYTES = LCD_HEIGHT * LCD_BUFFER_LINES * 2;

// FT6336.
constexpr int TOUCH_INT = 3;
constexpr uint8_t TOUCH_ADDR = 0x38;
// FT6336 on this panel reports the sensor in landscape coordinates (up to 480x320),
// while LVGL exposes the LCD in portrait coordinates (320x480).
// Mapping below is the board orientation used by the supplied LCD/touch example.
constexpr bool TOUCH_SWAP_XY = false;
constexpr bool TOUCH_INVERT_X = false;
constexpr bool TOUCH_INVERT_Y = false;
constexpr uint16_t TOUCH_RAW_WIDTH = 320;
constexpr uint16_t TOUCH_RAW_HEIGHT = 480;

// CH32V003 board I/O expander.
constexpr uint8_t IO_ADDR = 0x24;
constexpr uint8_t IO_MODE_REG = 0x02;
constexpr uint8_t IO_OUTPUT_REG = 0x03;
constexpr uint8_t IO_PWM_REG = 0x05;
constexpr uint8_t IO_RESET_TOUCH = 0;
constexpr uint8_t IO_RESET_LCD = 1;
constexpr uint8_t IO_POWER = 5;

// Shared SPI2 TF/SD card.
constexpr int SD_MISO = 2;
constexpr int SD_MOSI = 7;
constexpr int SD_CLK = 6;
constexpr int SD_CS = 9;
constexpr const char *SD_MOUNT = "/sd_card";
constexpr const char *SD_LOG_FILE = "/sd_card/sensors.csv";
constexpr bool SD_FORMAT_ON_FAIL = false;
constexpr uint32_t SD_FREQ_KHZ = 20000;

// Onboard devices.
constexpr uint8_t AXP2101_ADDR = 0x34;
constexpr uint8_t SHTC3_ADDR = 0x70;
constexpr uint8_t QMI8658_ADDR = 0x6B;
constexpr uint8_t PCF85063_ADDR = 0x51;

// UI / service cadence.
constexpr uint32_t SENSOR_UPDATE_MS = 700;
constexpr uint32_t RTC_UPDATE_MS = 500;
constexpr uint32_t POWER_UPDATE_MS = 1000;
constexpr uint32_t SD_LOG_INTERVAL_MS = 15000;
constexpr uint32_t UI_REFRESH_MS = 350;
constexpr uint32_t WIFI_SERVICE_MS = 350;

// Harmless, board-safe user outputs. 0/1/5 remain reserved for LCD/touch/power.
constexpr uint8_t USER_IO_PINS[] = {2, 3, 4, 6, 7};
constexpr size_t USER_IO_PIN_COUNT = sizeof(USER_IO_PINS) / sizeof(USER_IO_PINS[0]);

// Spain / mainland DST rules. NTP servers can be overridden later in settings.
constexpr const char *TZ_RULE = "CET-1CEST,M3.5.0/2,M10.5.0/3";
constexpr const char *NTP_1 = "pool.ntp.org";
constexpr const char *NTP_2 = "time.google.com";
constexpr const char *NTP_3 = "time.cloudflare.com";
}
