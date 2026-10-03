#include "app.h"

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <time.h>

#include "app_state.h"
#include "board_io.h"
#include "config.h"
#include "display.h"
#include "power_pmu.h"
#include "sd_storage.h"
#include "sensor_imu.h"
#include "sensor_rtc.h"
#include "sensor_shtc3.h"
#include "settings.h"
#include "time_manager.h"
#include "touch_ft6336.h"
#include "ui.h"
#include "wifi_manager.h"

C5Application app;

namespace {
static void haltWithMessage(const char *message) {
    Serial.println(message);
    while (true) delay(1000);
}

static String makeTimestamp() {
    if (!timeManager.valid()) return "1970-01-01 00:00:00";
    char buf[24]{};
    snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u:%02u",
             g_state.year, g_state.month, g_state.day,
             g_state.hour, g_state.minute, g_state.second);
    return String(buf);
}
}

bool C5Application::begin() {
    Serial.begin(C5Config::SERIAL_BAUD);
    delay(300);
    Serial.setTimeout(20);
    Serial.println();
    Serial.println("================================================");
    Serial.println(" SALARINI TOOL // ESP32-C5 ALL-IN-ONE SHELL");
    Serial.println("================================================");

    settingsStore.begin();

    if (!boardIO.begin(Wire)) haltWithMessage("FATAL: board I/O expander initialization failed");

    g_state.touchOk = touch.begin();

    // SD is mounted first using the board's documented ESP-VFS SDSPI example.
    // Use the board SDSPI mount path and keep the shared SPI2 transfer size safe for the LCD.
    g_state.sdOk = sdStorage.beginOnSharedSpiBus();
    if (g_state.sdOk) g_state.sdCapacityGb = sdStorage.capacityGb();

    if (!display.beginBus() || !display.beginPanel()) haltWithMessage("FATAL: ST7796/LVGL initialization failed");
    g_state.displayOk = display.ready();
    if (g_state.touchOk) {
        g_state.touchOk = touch.bindLvgl();
    }
    boardIO.setBacklight(settingsStore.brightness());

    // Devices on the already initialized shared I2C bus.
    g_state.shtc3Ok = shtc3.begin();
    g_state.imuOk = imu.begin();
    g_state.rtcOk = rtcSensor.begin();
    g_state.pmuOk = pmu.begin();

    timeManager.begin();
    wifiManager.begin();

    g_state.ioOutputBits = boardIO.outputBits();
    updateIo();
    updateSensors();
    updatePower();

    if (!ui.begin()) haltWithMessage("FATAL: UI initialization failed");

    const unsigned long now = millis();
    lastSensorMs_ = now;
    lastRtcMs_ = now;
    lastPowerMs_ = now;
    lastIoMs_ = now;
    lastUiMs_ = now;
    lastSdLogMs_ = now;
    lastPmuIrqMs_ = now;
    lastLvglMs_ = now;
    lastWifiMs_ = now;
    lastTimeMs_ = now;

    initialized_ = true;
    Serial.println("System ready. Type 'help' for terminal commands.");
    return true;
}

void C5Application::updateSensors() {
    if (shtc3.ready()) {
        g_state.shtc3Ok = shtc3.update();
        if (g_state.shtc3Ok) {
            g_state.temperatureC = shtc3.temperatureC();
            g_state.humidityPct = shtc3.humidityPct();
        }
    }

    if (imu.ready()) {
        g_state.imuOk = imu.update();
        if (g_state.imuOk) {
            g_state.accelX = imu.ax(); g_state.accelY = imu.ay(); g_state.accelZ = imu.az();
            g_state.gyroX = imu.gx(); g_state.gyroY = imu.gy(); g_state.gyroZ = imu.gz();
            g_state.imuTempC = imu.temperatureC(); g_state.steps = imu.steps();
        }
    }
}

void C5Application::updateRtc() {
    g_state.rtcOk = rtcSensor.update();
}

void C5Application::updatePower() {
    g_state.pmuOk = pmu.ready() && pmu.update();
    if (g_state.pmuOk) {
        g_state.batteryConnected = pmu.batteryConnected();
        g_state.charging = pmu.charging();
        g_state.discharging = pmu.discharging();
        g_state.batteryPercent = constrain(pmu.batteryPercent(), 0, 100);
        g_state.batteryMv = pmu.batteryMv();
        g_state.vbusMv = pmu.vbusMv();
        g_state.systemMv = pmu.systemMv();
        g_state.pmuTempC = pmu.temperatureC();
    }
}

void C5Application::updateIo() {
    uint16_t inputBits = 0;
    if (boardIO.readInputs(inputBits)) g_state.ioInputBits = inputBits;
    g_state.ioAdc = boardIO.readAdc();
    g_state.ioOutputBits = boardIO.outputBits();
}

void C5Application::logToSd() {
    if (!sdStorage.ready()) { g_state.lastSdLogOk = false; return; }
    char line[320]{};
    snprintf(line, sizeof(line), "%s,%.2f,%.2f,%.4f,%.4f,%.4f,%.3f,%.3f,%.3f,%u,%u,%u\n",
             makeTimestamp().c_str(), g_state.temperatureC, g_state.humidityPct,
             g_state.accelX, g_state.accelY, g_state.accelZ,
             g_state.gyroX, g_state.gyroY, g_state.gyroZ,
             g_state.batteryPercent, g_state.batteryMv, g_state.vbusMv);
    g_state.lastSdLogOk = sdStorage.appendText(C5Config::SD_LOG_FILE, line) == ESP_OK;
}

void C5Application::processSerial() {
    if (!Serial.available()) return;
    String command = Serial.readStringUntil('\n'); command.trim(); if (!command.length()) return;

    if (command.equalsIgnoreCase("help")) {
        Serial.println("help | status | wifi scan | wifi info | wifi off | sync | time | sdtest | i2cscan | backlight <0..100> | time YYYY MM DD HH MM SS | reboot");
        return;
    }
    if (command.equalsIgnoreCase("status")) {
        Serial.printf("SHTC3=%s IMU=%s RTC=%s PMU=%s SD=%s WIFI=%s NTP=%s\n",
                      g_state.shtc3Ok?"OK":"OFF", g_state.imuOk?"OK":"OFF", g_state.rtcOk?"OK":"OFF",
                      g_state.pmuOk?"OK":"OFF", g_state.sdOk?"OK":"OFF", g_state.wifiConnected?"ON":"OFF", g_state.timeSynced?"SYNC":"NO");
        Serial.printf("TIME=%s BAT=%u%% %umV TEMP=%.2fC RH=%.2f%%\n",
                      makeTimestamp().c_str(), g_state.batteryPercent, g_state.batteryMv,
                      g_state.temperatureC, g_state.humidityPct);
        return;
    }
    if (command.equalsIgnoreCase("wifi scan")) { wifiManager.startScan(); Serial.println("WiFi scan started"); return; }
    if (command.equalsIgnoreCase("wifi info")) { Serial.printf("SSID=%s IP=%s RSSI=%lddBm GW=%s MAC=%s\n", wifiManager.ssid().c_str(), wifiManager.ip().c_str(), static_cast<long>(wifiManager.rssi()), wifiManager.gateway().c_str(), wifiManager.mac().c_str()); return; }
    if (command.equalsIgnoreCase("wifi off")) { wifiManager.disconnect(); Serial.println("WiFi disconnected"); return; }
    if (command.equalsIgnoreCase("sync")) { Serial.println(timeManager.syncNtp() ? "NTP sync OK" : "NTP sync FAILED"); return; }
    if (command.equalsIgnoreCase("time")) { Serial.println(timeManager.formatted()); return; }
    if (command.equalsIgnoreCase("i2cscan")) {
        String result = "I2C:";
        for (uint8_t a=3;a<0x78;++a) { Wire.beginTransmission(a); if (Wire.endTransmission()==0) { char b[6]; snprintf(b,sizeof(b)," %02X",a); result += b; } }
        Serial.println(result); return;
    }
    if (command.equalsIgnoreCase("sdtest")) { const esp_err_t e=sdStorage.writeText("/sd_card/serial_test.txt","SALARINI SERIAL SD OK\n"); Serial.println(e==ESP_OK?"SD OK":esp_err_to_name(e)); return; }
    int value=0;
    if (sscanf(command.c_str(), "backlight %d", &value)==1) { value=constrain(value,0,100); settingsStore.setBrightness(value); Serial.println(boardIO.setBacklight(value)?"Backlight OK":"Backlight FAIL"); return; }
    int y=0,mo=0,d=0,h=0,mi=0,s=0;
    if (sscanf(command.c_str(), "time %d %d %d %d %d %d", &y,&mo,&d,&h,&mi,&s)==6) { Serial.println(timeManager.setManual(String(y)+"-"+String(mo)+"-"+String(d)+" "+String(h)+":"+String(mi)+":"+String(s))?"RTC SET OK":"RTC SET FAIL"); return; }
    if (command.equalsIgnoreCase("reboot")) { ESP.restart(); }
    Serial.printf("Unknown command: %s\n", command.c_str());
}

void C5Application::loop() {
    if (!initialized_) return;
    const unsigned long now = millis();

    if (now - lastSensorMs_ >= C5Config::SENSOR_UPDATE_MS) { lastSensorMs_ = now; updateSensors(); }
    if (now - lastRtcMs_ >= C5Config::RTC_UPDATE_MS) { lastRtcMs_ = now; updateRtc(); }
    if (now - lastPowerMs_ >= C5Config::POWER_UPDATE_MS) { lastPowerMs_ = now; updatePower(); }
    if (now - lastIoMs_ >= C5Config::SENSOR_UPDATE_MS) { lastIoMs_ = now; updateIo(); }
    if (now - lastPmuIrqMs_ >= 250) { lastPmuIrqMs_ = now; pmu.pollIrqEvents(); }
    if (now - lastSdLogMs_ >= C5Config::SD_LOG_INTERVAL_MS) { lastSdLogMs_ = now; logToSd(); g_state.sdOk = sdStorage.ready(); }
    if (now - lastWifiMs_ >= C5Config::WIFI_SERVICE_MS) {
        lastWifiMs_ = now;
        wifiManager.service();
        g_state.wifiReady = true;
        g_state.wifiConnected = wifiManager.connected();
        g_state.wifiConnecting = wifiManager.connecting();
        g_state.wifiScanning = wifiManager.scanInProgress();
        g_state.wifiSsid = wifiManager.ssid();
        g_state.wifiRssi = wifiManager.rssi();
        g_state.wifiIp = wifiManager.ip();
        g_state.wifiGateway = wifiManager.gateway();
        g_state.wifiMac = wifiManager.mac();
    }
    if (now - lastTimeMs_ >= C5Config::RTC_UPDATE_MS) {
        lastTimeMs_ = now;
        timeManager.service(g_state.wifiConnected);
    }
    if (now - lastUiMs_ >= C5Config::UI_REFRESH_MS) { lastUiMs_ = now; ui.update(); }
    if (now - lastLvglMs_ >= 20) { const uint32_t elapsed = static_cast<uint32_t>(now-lastLvglMs_); lastLvglMs_=now; display.tick(elapsed); }

    g_state.uptimeSeconds = millis()/1000UL;
    processSerial();
}
