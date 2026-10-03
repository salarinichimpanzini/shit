#include "settings.h"

SettingsStore settingsStore;

bool SettingsStore::begin() {
    ready_ = prefs_.begin("salarini", false);
    if (!ready_) return false;
    brightness_ = prefs_.getUChar("brightness", 100);
    if (brightness_ > 100) brightness_ = 100;
    lastEpoch_ = prefs_.getULong("epoch", 0);
    rotation_ = prefs_.getUChar("rotation", 0);
    if (rotation_ > 3) rotation_ = 0;
    return true;
}

void SettingsStore::setBrightness(uint8_t percent) {
    brightness_ = percent > 100 ? 100 : percent;
    if (ready_) prefs_.putUChar("brightness", brightness_);
}

void SettingsStore::setLastEpoch(uint32_t epoch) {
    lastEpoch_ = epoch;
    if (ready_) prefs_.putULong("epoch", epoch);
}

void SettingsStore::setRotation(uint8_t rotation) {
    rotation_ = rotation & 0x03;
    if (ready_) prefs_.putUChar("rotation", rotation_);
}
