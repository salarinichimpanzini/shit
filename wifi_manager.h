#pragma once

#include <Arduino.h>
#include <WiFi.h>

struct WifiNetworkInfo {
    String ssid;
    int32_t rssi = 0;
    int32_t channel = 0;
    bool open = false;
    String bssid;
};

class WifiManager {
public:
    void begin();
    void service();
    void startScan();
    int scanCount() const;
    WifiNetworkInfo network(int index) const;
    void connect(const String &ssid, const String &password);
    void disconnect();
    bool connected() const { return stateConnected_; }
    bool connecting() const { return stateConnecting_; }
    String ssid() const;
    String ip() const;
    String gateway() const;
    String mac() const;
    int32_t rssi() const;
    bool justConnected();
    bool scanInProgress() const { return scanInProgress_; }
    String scanStatus() const { return scanStatus_; }

private:
    bool stateConnected_ = false;
    bool stateConnecting_ = false;
    bool justConnected_ = false;
    bool scanInProgress_ = false;
    int scanCount_ = 0;
    int lastStatus_ = WL_NO_SHIELD;
    String selectedSsid_;
    String scanStatus_ = "IDLE";
    unsigned long connectStartedMs_ = 0;
};

extern WifiManager wifiManager;
