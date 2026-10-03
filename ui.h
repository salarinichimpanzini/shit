#pragma once

#include <Arduino.h>
#define LV_CONF_INCLUDE_SIMPLE
#include <lvgl.h>
#include "soft_keyboard.h"

class AppUi {
public:
    bool begin();
    void update();

private:
    enum View : uint8_t {
        VIEW_ROOT,
        VIEW_EXPLOIT,
        VIEW_DEAUTH,
        VIEW_APS,
        VIEW_STAS,
        VIEW_EVIL_TWIN,
        VIEW_COOL_STUFF,
        VIEW_BEACON,
        VIEW_AP_SPOOF,
        VIEW_BT_ADV,
        VIEW_SCAN,
        VIEW_AP_DETAIL,
        VIEW_WIFI_CONNECT,
        VIEW_PACKET_MONITOR,
        VIEW_SETTINGS,
        VIEW_ABOUT
    };

    bool ready_ = false;
    bool selectingForConnect_ = false;
    View view_ = VIEW_ROOT;
    View previousView_ = VIEW_ROOT;

    lv_obj_t *root_ = nullptr;
    lv_obj_t *header_ = nullptr;
    lv_obj_t *backButton_ = nullptr;
    lv_obj_t *content_ = nullptr;
    lv_obj_t *title_ = nullptr;
    lv_obj_t *clock_ = nullptr;
    lv_obj_t *battery_ = nullptr;
    lv_obj_t *pageTitle_ = nullptr;
    lv_obj_t *pageHint_ = nullptr;
    lv_obj_t *status_ = nullptr;
    lv_obj_t *scanList_ = nullptr;
    lv_obj_t *scanInfo_ = nullptr;
    lv_obj_t *settingsInfo_ = nullptr;
    lv_obj_t *aboutInfo_ = nullptr;
    lv_obj_t *packetInfo_ = nullptr;
    lv_obj_t *brightnessSlider_ = nullptr;
    lv_obj_t *wifiPasswordField_ = nullptr;
    lv_obj_t *wifiStatus_ = nullptr;
    lv_obj_t *packetBars_[14]{};
    lv_obj_t *packetBarLabels_[14]{};

    lv_timer_t *uiTimer_ = nullptr;
    uint32_t uiPhase_ = 0;
    uint32_t scanAnimUntil_ = 0;
    uint32_t labAnimUntil_ = 0;
    uint32_t packetNextScanMs_ = 0;
    int lastScanCount_ = -1;
    int selectedWifiIndex_ = -1;
    String selectedSsid_;

    static void navEvent(lv_event_t *e);
    static void backEvent(lv_event_t *e);
    static void scanEvent(lv_event_t *e);
    static void wifiSelectEvent(lv_event_t *e);
    static void wifiConnectEvent(lv_event_t *e);
    static void wifiDisconnectEvent(lv_event_t *e);
    static void openInputEvent(lv_event_t *e);
    static void brightnessEvent(lv_event_t *e);
    static void rotationEvent(lv_event_t *e);
    static void ntpEvent(lv_event_t *e);
    static void safeLabEvent(lv_event_t *e);
    static void connectWifiEvent(lv_event_t *e);
    static void timerCb(lv_timer_t *t);
    static void keyboardVisibility(bool visible);

    void show(View view, bool push = true);
    void clearPage();
    void buildRoot();
    void buildExploit();
    void buildDeauth();
    void buildAps();
    void buildStas();
    void buildEvilTwin();
    void buildCoolStuff();
    void buildBeacon();
    void buildApSpoof();
    void buildBtAdv();
    void buildScan();
    void buildApDetail();
    void buildWifiConnect();
    void buildPacketMonitor();
    void buildSettings();
    void buildAbout();

    lv_obj_t *makeText(lv_obj_t *parent, const char *text, uint32_t color = 0xF4F7F8, int width = -1, uint8_t fontSize = 14);
    lv_obj_t *makeRow(const char *label, View target, uint32_t accent = 0xA6E3FF, const char *tag = nullptr);
    lv_obj_t *makeAction(const char *label, void (*cb)(lv_event_t *), void *data = nullptr, uint32_t accent = 0xA6E3FF);
    lv_obj_t *makeField(const char *placeholder, SoftKeyboard::Mode mode, lv_obj_t **store, bool passwordMode = false);
    void makeHeader();
    void refreshHeader();
    void refreshPage();
    void renderScanResults();
    void renderPacketGraph();
    void openKeyboard(lv_obj_t *target, SoftKeyboard::Mode mode);
    void rebuildForRotation();
    String scanSummary() const;
    String channelSummary() const;
    String sensorSummary() const;
    void setStatus(const String &s);
    void animatePageEntry();
    void setContentHeightForKeyboard(bool visible);
};

extern AppUi ui;
