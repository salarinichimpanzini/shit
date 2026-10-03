#pragma once

#include <Arduino.h>
#include <esp_err.h>

class SdStorage {
public:
    bool beginOnSharedSpiBus();
    bool ready() const { return ready_; }
    float capacityGb() const { return capacityGb_; }
    esp_err_t writeText(const char *path, const char *text);
    esp_err_t appendText(const char *path, const char *text);
    bool exists(const char *path) const;

private:
    bool ready_ = false;
    float capacityGb_ = 0.0f;
    void *card_ = nullptr;
    bool ensureLogFile();
};

extern SdStorage sdStorage;
