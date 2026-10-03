#pragma once

#include <Arduino.h>
#include <Preferences.h>

class SettingsStore {
public:
    bool begin();
    uint8_t brightness() const { return brightness_; }
    void setBrightness(uint8_t percent);
    uint32_t lastEpoch() const { return lastEpoch_; }
    void setLastEpoch(uint32_t epoch);
    uint8_t rotation() const { return rotation_; }
    void setRotation(uint8_t rotation);

private:
    Preferences prefs_;
    bool ready_ = false;
    uint8_t brightness_ = 100;
    uint32_t lastEpoch_ = 0;
    uint8_t rotation_ = 0;
};

extern SettingsStore settingsStore;
