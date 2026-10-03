#pragma once

#include <Arduino.h>

class TimeManager {
public:
    bool begin();
    void service(bool wifiConnected);
    bool syncNtp();
    bool setManual(const String &text);
    bool setRtcFromSystemTime();
    bool valid() const { return valid_; }
    bool synced() const { return synced_; }
    String formatted() const;

private:
    bool valid_ = false;
    bool synced_ = false;
    bool syncInProgress_ = false;
    unsigned long lastSyncAttemptMs_ = 0;
    String lastError_;
    void refreshState();
};

extern TimeManager timeManager;
