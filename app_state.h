#pragma once

#include <Arduino.h>

struct AppState {
    // Devices
    bool shtc3Ok = false;
    bool imuOk = false;
    bool rtcOk = false;
    bool pmuOk = false;
    bool sdOk = false;
    bool touchOk = false;
    bool displayOk = false;

    // Environment / IMU
    float temperatureC = 0.0f;
    float humidityPct = 0.0f;
    float accelX = 0.0f;
    float accelY = 0.0f;
    float accelZ = 0.0f;
    float gyroX = 0.0f;
    float gyroY = 0.0f;
    float gyroZ = 0.0f;
    float imuTempC = 0.0f;
    uint32_t steps = 0;

    // PMU
    bool batteryConnected = false;
    bool charging = false;
    bool discharging = false;
    uint8_t batteryPercent = 0;
    uint16_t batteryMv = 0;
    uint16_t vbusMv = 0;
    uint16_t systemMv = 0;
    float pmuTempC = 0.0f;
    float sdCapacityGb = 0.0f;

    // RTC / system time
    uint16_t year = 0;
    uint8_t month = 0;
    uint8_t day = 0;
    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;
    uint8_t week = 0;
    bool timeSynced = false;

    // I/O expander
    uint16_t ioInputBits = 0;
    uint16_t ioOutputBits = 0;
    uint16_t ioAdc = 0;

    // Wi-Fi
    bool wifiReady = false;
    bool wifiConnected = false;
    bool wifiConnecting = false;
    bool wifiScanning = false;
    String wifiSsid;
    int32_t wifiRssi = 0;
    String wifiIp;
    String wifiGateway;
    String wifiMac;

    // Runtime
    uint32_t uptimeSeconds = 0;
    uint32_t uiInteractions = 0;
    bool lastSdLogOk = false;
};

extern AppState g_state;
