#include "wifi_manager.h"

WifiManager wifiManager;

void WifiManager::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    stateConnected_ = WiFi.status() == WL_CONNECTED;
    stateConnecting_ = false;
    scanStatus_ = "READY";
}

void WifiManager::service() {
    const int s = WiFi.status();
    const bool nowConnected = s == WL_CONNECTED;
    justConnected_ = (!stateConnected_ && nowConnected);
    stateConnected_ = nowConnected;

    if (stateConnecting_) {
        if (nowConnected) {
            stateConnecting_ = false;
        } else if (connectStartedMs_ && millis() - connectStartedMs_ > 20000UL) {
            stateConnecting_ = false;
        }
    }

    if (scanInProgress_) {
        const int result = WiFi.scanComplete();
        if (result >= 0) {
            scanCount_ = result;
            scanInProgress_ = false;
            scanStatus_ = String(result) + " NETWORKS";
        } else if (result == WIFI_SCAN_FAILED) {
            scanInProgress_ = false;
            scanCount_ = 0;
            scanStatus_ = "SCAN FAILED";
        }
    }
}

void WifiManager::startScan() {
    if (scanInProgress_) return;
    WiFi.scanDelete();
    scanCount_ = 0;
    scanStatus_ = "SCANNING...";
    scanInProgress_ = WiFi.scanNetworks(true, true) == WIFI_SCAN_RUNNING;
    if (!scanInProgress_) {
        // Some core versions return the number immediately in synchronous mode.
        const int result = WiFi.scanComplete();
        if (result >= 0) {
            scanCount_ = result;
            scanStatus_ = String(result) + " NETWORKS";
        }
    }
}

int WifiManager::scanCount() const { return scanCount_; }

WifiNetworkInfo WifiManager::network(int index) const {
    WifiNetworkInfo info;
    if (index < 0 || index >= scanCount_) return info;
    info.ssid = WiFi.SSID(index);
    info.rssi = WiFi.RSSI(index);
    info.open = WiFi.encryptionType(index) == WIFI_AUTH_OPEN;
    info.channel = WiFi.channel(index);
    info.bssid = WiFi.BSSIDstr(index);
    return info;
}

void WifiManager::connect(const String &ssid, const String &password) {
    if (ssid.isEmpty()) return;
    selectedSsid_ = ssid;
    stateConnecting_ = true;
    connectStartedMs_ = millis();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
}

void WifiManager::disconnect() {
    WiFi.disconnect(false, false);
    stateConnected_ = false;
    stateConnecting_ = false;
}

String WifiManager::ssid() const { return stateConnected_ ? WiFi.SSID() : selectedSsid_; }
String WifiManager::ip() const { return stateConnected_ ? WiFi.localIP().toString() : String("-"); }
String WifiManager::gateway() const { return stateConnected_ ? WiFi.gatewayIP().toString() : String("-"); }
String WifiManager::mac() const { return WiFi.macAddress(); }
int32_t WifiManager::rssi() const { return stateConnected_ ? WiFi.RSSI() : 0; }

bool WifiManager::justConnected() {
    const bool v = justConnected_;
    justConnected_ = false;
    return v;
}
