#include "ui.h"

#include <WiFi.h>
#include <ESP.h>

#include "app_state.h"
#include "board_io.h"
#include "display.h"
#include "power_pmu.h"
#include "sd_storage.h"
#include "settings.h"
#include "time_manager.h"
#include "wifi_manager.h"

namespace {
constexpr uint32_t BLACK = 0x000000;
constexpr uint32_t PANEL = 0x0A0D10;
constexpr uint32_t PANEL2 = 0x10161B;
constexpr uint32_t WHITE = 0xF5F7F8;
constexpr uint32_t DIM = 0x6D7C86;
constexpr uint32_t CYAN = 0x8DEBFF;
constexpr uint32_t GREEN = 0x71F6A5;
constexpr uint32_t AMBER = 0xF2D06B;
constexpr uint32_t RED = 0xFF6678;
constexpr uint32_t MAGENTA = 0xE3A1FF;
constexpr uint32_t BLUE = 0x8DB8FF;
constexpr uint32_t LINE = 0x273139;
constexpr uint32_t PRESS = 0x1A2830;
constexpr uint16_t HEADER_H = 48;
constexpr uint16_t ROW_H = 40;
constexpr uint16_t GAP = 6;

static void resetObj(lv_obj_t *o) {
    lv_obj_remove_style_all(o);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_CHAIN | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
}

static void stylePanel(lv_obj_t *o, uint32_t bg = PANEL, uint32_t border = LINE) {
    resetObj(o);
    lv_obj_set_style_bg_color(o, lv_color_hex(bg), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(o, lv_color_hex(border), 0);
    lv_obj_set_style_border_width(o, 1, 0);
    lv_obj_set_style_radius(o, 3, 0);
    lv_obj_set_style_pad_all(o, 7, 0);
}

static void styleButton(lv_obj_t *b, uint32_t accent) {
    resetObj(b);
    lv_obj_set_style_bg_color(b, lv_color_hex(PANEL), 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(PRESS), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(b, lv_color_hex(LINE), 0);
    lv_obj_set_style_border_width(b, 1, 0);
    lv_obj_set_style_border_color(b, lv_color_hex(accent), LV_STATE_FOCUSED);
    lv_obj_set_style_radius(b, 3, 0);
    lv_obj_set_style_pad_left(b, 9, 0);
    lv_obj_set_style_pad_right(b, 9, 0);
    lv_obj_set_style_text_color(b, lv_color_hex(WHITE), 0);
    lv_obj_set_style_text_color(b, lv_color_hex(accent), LV_STATE_PRESSED);
}

static uint32_t clampColor(uint32_t c) { return c & 0xFFFFFFU; }
}

AppUi ui;

bool AppUi::begin() {
    if (!display.ready()) return false;
    root_ = lv_scr_act();
    resetObj(root_);
    lv_obj_set_style_bg_color(root_, lv_color_hex(BLACK), 0);
    lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);

    makeHeader();
    content_ = lv_obj_create(root_);
    stylePanel(content_, BLACK, BLACK);
    lv_obj_set_width(content_, display.width());
    lv_obj_set_height(content_, max(80, static_cast<int>(display.height()) - HEADER_H));
    lv_obj_set_pos(content_, 0, HEADER_H);
    lv_obj_set_style_pad_left(content_, 8, 0);
    lv_obj_set_style_pad_right(content_, 8, 0);
    lv_obj_set_style_pad_top(content_, 7, 0);
    lv_obj_set_style_pad_bottom(content_, 10, 0);
    lv_obj_set_style_pad_row(content_, GAP, 0);
    lv_obj_set_flex_flow(content_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_add_flag(content_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(content_, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(content_, LV_SCROLLBAR_MODE_AUTO);

    softKeyboard.setVisibilityCallback(keyboardVisibility);
    uiTimer_ = lv_timer_create(timerCb, 240, nullptr);
    ready_ = true;
    show(VIEW_ROOT, false);
    refreshHeader();
    return true;
}

void AppUi::makeHeader() {
    header_ = lv_obj_create(root_);
    stylePanel(header_, BLACK, LINE);
    lv_obj_set_width(header_, display.width());
    lv_obj_set_height(header_, HEADER_H);
    lv_obj_set_pos(header_, 0, 0);
    lv_obj_set_style_pad_all(header_, 5, 0);

    backButton_ = lv_btn_create(header_);
    styleButton(backButton_, WHITE);
    lv_obj_set_size(backButton_, 48, 34);
    lv_obj_set_pos(backButton_, 5, 6);
    lv_obj_add_event_cb(backButton_, backEvent, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *backLabel = lv_label_create(backButton_);
    lv_label_set_text(backLabel, "<");
    lv_obj_center(backLabel);

    title_ = lv_label_create(header_);
    lv_obj_set_style_text_color(title_, lv_color_hex(CYAN), 0);
    lv_label_set_text(title_, "SALARINI TOOL");
    lv_obj_set_pos(title_, 62, 7);
    lv_obj_set_width(title_, max(80, static_cast<int>(display.width()) - 210));
    lv_label_set_long_mode(title_, LV_LABEL_LONG_CLIP);

    clock_ = lv_label_create(header_);
    lv_obj_set_style_text_color(clock_, lv_color_hex(WHITE), 0);
    lv_label_set_text(clock_, "--:--:--");
    lv_obj_set_width(clock_, 74);
    lv_obj_align(clock_, LV_ALIGN_TOP_RIGHT, -78, 7);
    lv_label_set_long_mode(clock_, LV_LABEL_LONG_CLIP);

    battery_ = lv_label_create(header_);
    lv_obj_set_style_text_color(battery_, lv_color_hex(GREEN), 0);
    lv_label_set_text(battery_, "BAT --%");
    lv_obj_set_width(battery_, 68);
    lv_obj_align(battery_, LV_ALIGN_TOP_RIGHT, -5, 7);
}

lv_obj_t *AppUi::makeText(lv_obj_t *parent, const char *text, uint32_t color, int width, uint8_t) {
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text ? text : "");
    lv_obj_set_style_text_color(l, lv_color_hex(clampColor(color)), 0);
    lv_obj_set_style_pad_all(l, 0, 0);
    if (width > 0) {
        lv_obj_set_width(l, width);
        lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
    }
    return l;
}

lv_obj_t *AppUi::makeRow(const char *label, View target, uint32_t accent, const char *tag) {
    lv_obj_t *b = lv_btn_create(content_);
    styleButton(b, accent);
    lv_obj_set_width(b, LV_PCT(100));
    lv_obj_set_height(b, ROW_H);
    lv_obj_add_event_cb(b, navEvent, LV_EVENT_CLICKED, reinterpret_cast<void *>(static_cast<intptr_t>(target)));

    lv_obj_t *main = lv_label_create(b);
    lv_label_set_text(main, label ? label : "");
    lv_obj_set_style_text_color(main, lv_color_hex(WHITE), 0);
    lv_obj_set_width(main, tag ? max(50, static_cast<int>(display.width()) - 140) : LV_PCT(100));
    lv_label_set_long_mode(main, LV_LABEL_LONG_CLIP);
    lv_obj_align(main, LV_ALIGN_LEFT_MID, 0, 0);

    if (tag && tag[0]) {
        lv_obj_t *t = lv_label_create(b);
        lv_label_set_text(t, tag);
        lv_obj_set_style_text_color(t, lv_color_hex(DIM), 0);
        lv_obj_set_width(t, 108);
        lv_label_set_long_mode(t, LV_LABEL_LONG_CLIP);
        lv_obj_align(t, LV_ALIGN_RIGHT_MID, 0, 0);
    }
    return b;
}

lv_obj_t *AppUi::makeAction(const char *label, void (*cb)(lv_event_t *), void *data, uint32_t accent) {
    lv_obj_t *b = lv_btn_create(content_);
    styleButton(b, accent);
    lv_obj_set_width(b, LV_PCT(100));
    lv_obj_set_height(b, ROW_H);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, data);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, label ? label : "");
    lv_obj_set_style_text_color(l, lv_color_hex(accent), 0);
    lv_label_set_long_mode(l, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(l, LV_PCT(100));
    lv_obj_center(l);
    return b;
}

lv_obj_t *AppUi::makeField(const char *placeholder, SoftKeyboard::Mode mode, lv_obj_t **store, bool passwordMode) {
    lv_obj_t *ta = lv_textarea_create(content_);
    lv_obj_set_width(ta, LV_PCT(100));
    lv_obj_set_height(ta, ROW_H);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_placeholder_text(ta, placeholder ? placeholder : "INPUT");
    lv_textarea_set_password_mode(ta, passwordMode);
    lv_obj_set_style_bg_color(ta, lv_color_hex(PANEL), 0);
    lv_obj_set_style_bg_opa(ta, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ta, lv_color_hex(LINE), 0);
    lv_obj_set_style_border_width(ta, 1, 0);
    lv_obj_set_style_radius(ta, 3, 0);
    lv_obj_set_style_text_color(ta, lv_color_hex(WHITE), 0);
    lv_obj_set_style_text_color(ta, lv_color_hex(DIM), LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_add_event_cb(ta, openInputEvent, LV_EVENT_CLICKED, reinterpret_cast<void *>(static_cast<intptr_t>(mode)));
    if (store) *store = ta;
    return ta;
}

void AppUi::clearPage() {
    softKeyboard.close();
    if (content_) lv_obj_clean(content_);
    pageTitle_ = nullptr;
    pageHint_ = nullptr;
    status_ = nullptr;
    scanList_ = nullptr;
    scanInfo_ = nullptr;
    settingsInfo_ = nullptr;
    aboutInfo_ = nullptr;
    packetInfo_ = nullptr;
    wifiPasswordField_ = nullptr;
    wifiStatus_ = nullptr;
    brightnessSlider_ = nullptr;
    for (auto &p : packetBars_) p = nullptr;
    for (auto &p : packetBarLabels_) p = nullptr;
}

void AppUi::show(View view, bool push) {
    if (!ready_ && view != VIEW_ROOT) return;
    if (push && view != view_) previousView_ = view_;
    view_ = view;
    clearPage();
    switch (view_) {
        case VIEW_ROOT: buildRoot(); break;
        case VIEW_EXPLOIT: buildExploit(); break;
        case VIEW_DEAUTH: buildDeauth(); break;
        case VIEW_APS: buildAps(); break;
        case VIEW_STAS: buildStas(); break;
        case VIEW_EVIL_TWIN: buildEvilTwin(); break;
        case VIEW_COOL_STUFF: buildCoolStuff(); break;
        case VIEW_BEACON: buildBeacon(); break;
        case VIEW_AP_SPOOF: buildApSpoof(); break;
        case VIEW_BT_ADV: buildBtAdv(); break;
        case VIEW_SCAN: buildScan(); break;
        case VIEW_AP_DETAIL: buildApDetail(); break;
        case VIEW_WIFI_CONNECT: buildWifiConnect(); break;
        case VIEW_PACKET_MONITOR: buildPacketMonitor(); break;
        case VIEW_SETTINGS: buildSettings(); break;
        case VIEW_ABOUT: buildAbout(); break;
    }
    lv_obj_scroll_to_y(content_, 0, LV_ANIM_OFF);
    if (backButton_) {
        if (view_ == VIEW_ROOT) lv_obj_add_flag(backButton_, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_clear_flag(backButton_, LV_OBJ_FLAG_HIDDEN);
    }
    if (title_) lv_obj_set_x(title_, view_ == VIEW_ROOT ? 12 : 62);
    animatePageEntry();
    refreshHeader();
}

void AppUi::buildRoot() {
    pageTitle_ = makeText(content_, "SALARINI TOOL", CYAN, display.width() - 16);
    pageHint_ = makeText(content_, "security field interface / low resource mode", DIM, display.width() - 16);
    makeRow("|-- Exploit", VIEW_EXPLOIT, RED, "LAB");
    makeRow("|-- Scan", VIEW_SCAN, CYAN, "Wi-Fi");
    makeRow("|-- WiFi Connect", VIEW_WIFI_CONNECT, BLUE, "STA");
    makeRow("|-- Packet Monitor", VIEW_PACKET_MONITOR, GREEN, "PASSIVE");
    makeRow("|-- Settings", VIEW_SETTINGS, AMBER, "SYSTEM");
    makeRow("`-- About", VIEW_ABOUT, MAGENTA, "INFO");
}

void AppUi::buildExploit() {
    makeText(content_, "EXPLOIT", RED, display.width() - 16);
    makeText(content_, "lab navigation / simulation modules", DIM, display.width() - 16);
    makeRow("|-- Deauth", VIEW_DEAUTH, RED, "LAB");
    makeRow("|-- Evil Twin", VIEW_EVIL_TWIN, RED, "LAB");
    makeRow("|-- Cool Stuff", VIEW_COOL_STUFF, MAGENTA, "DEMO");
    makeRow("|-- Beacon", VIEW_BEACON, CYAN, "LAB");
    makeRow("|-- AP Spoofing", VIEW_AP_SPOOF, AMBER, "LAB");
    makeRow("`-- B. T Adv", VIEW_BT_ADV, GREEN, "BLE");
}

void AppUi::buildDeauth() {
    makeText(content_, "DEAUTH / LAB", RED, display.width() - 16);
    makeText(content_, "No frames are transmitted. Select and analyze a target only.", DIM, display.width() - 16);
    makeRow("|-- APs", VIEW_APS, RED, "TARGETS");
    makeRow("`-- STAs", VIEW_STAS, RED, "INVENTORY");
}

void AppUi::buildAps() {
    makeText(content_, "APs", CYAN, display.width() - 16);
    makeAction("ALL APs / OPEN SCAN", scanEvent, nullptr, CYAN);
    makeAction("CHANNEL / ANALYZER", scanEvent, nullptr, GREEN);
    makeAction("SELECT WIFI", scanEvent, nullptr, AMBER);
}

void AppUi::buildStas() {
    makeText(content_, "STAs", GREEN, display.width() - 16);
    makeText(content_, "Passive station discovery is hardware/driver dependent.", DIM, display.width() - 16);
    makeAction("STATION INVENTORY / LAB", safeLabEvent, nullptr, GREEN);
}

void AppUi::buildEvilTwin() {
    makeText(content_, "EVIL TWIN / LAB", RED, display.width() - 16);
    makeText(content_, "Select an AP for a visual simulation only.", DIM, display.width() - 16);
    makeAction("SELECT TARGET AP", scanEvent, nullptr, CYAN);
    makeAction("RUN LAB SIMULATION", safeLabEvent, nullptr, RED);
}

void AppUi::buildCoolStuff() {
    makeText(content_, "COOL STUFF", MAGENTA, display.width() - 16);
    makeAction("DEMO AIRPLAY", safeLabEvent, nullptr, MAGENTA);
    makeAction("GAME 1 / SIGNAL REACTION", safeLabEvent, nullptr, CYAN);
    makeAction("GAME 2 / CHANNEL DODGE", safeLabEvent, nullptr, GREEN);
}

void AppUi::buildBeacon() {
    makeText(content_, "BEACON / LAB", CYAN, display.width() - 16);
    makeText(content_, "Visual laboratory only. No fake beacon transmission.", DIM, display.width() - 16);
    makeAction("ALL SSIDs DUPE / SIM", safeLabEvent, nullptr, CYAN);
    makeAction("SELECTED WIFI DUPE / SIM", safeLabEvent, nullptr, CYAN);
    makeAction("RANDOM / SIM", safeLabEvent, nullptr, GREEN);
    makeAction("CHANNEL / SIM", safeLabEvent, nullptr, AMBER);
    makeAction("PREFIX / SIM", safeLabEvent, nullptr, MAGENTA);
}

void AppUi::buildApSpoof() {
    makeText(content_, "AP SPOOFING / LAB", AMBER, display.width() - 16);
    makeText(content_, "Defensive UI simulation. No spoofed AP is created.", DIM, display.width() - 16);
    makeAction("SELECTED TARGET / SIM", safeLabEvent, nullptr, AMBER);
}

void AppUi::buildBtAdv() {
    makeText(content_, "B. T ADV / LAB", GREEN, display.width() - 16);
    makeText(content_, "Advertising demos are placeholders until a dedicated BLE module is added.", DIM, display.width() - 16);
    makeAction("SAMSUNG / SIMULATION", safeLabEvent, nullptr, GREEN);
    makeAction("iOS / SIMULATION", safeLabEvent, nullptr, GREEN);
}

void AppUi::buildScan() {
    makeText(content_, "SCAN", CYAN, display.width() - 16);
    makeText(content_, "PASSIVE SURVEY / RSSI / CHANNEL / SECURITY", DIM, display.width() - 16);
    makeAction("START SCAN", scanEvent, nullptr, CYAN);
    scanInfo_ = makeText(content_, "READY", WHITE, display.width() - 16);
    scanList_ = lv_obj_create(content_);
    stylePanel(scanList_, BLACK, BLACK);
    lv_obj_set_width(scanList_, LV_PCT(100));
    lv_obj_set_height(scanList_, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(scanList_, 0, 0);
    lv_obj_set_style_pad_row(scanList_, 5, 0);
    lv_obj_set_flex_flow(scanList_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scanList_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(scanList_, LV_OBJ_FLAG_SCROLLABLE);
    lastScanCount_ = -1;
    renderScanResults();
}

void AppUi::buildApDetail() {
    makeText(content_, "AP DETAIL", CYAN, display.width() - 16);
    if (selectedWifiIndex_ < 0 || selectedWifiIndex_ >= wifiManager.scanCount()) {
        makeText(content_, "No AP selected. Return to SCAN.", DIM, display.width() - 16);
        return;
    }
    const WifiNetworkInfo n = wifiManager.network(selectedWifiIndex_);
    String ssid = n.ssid.length() ? n.ssid : String("<hidden>");
    makeText(content_, ssid.c_str(), WHITE, display.width() - 16);
    String meta;
    meta += "RSSI  " + String(n.rssi) + " dBm\n";
    meta += "CH    " + String(n.channel) + "\n";
    meta += "SEC   " + String(n.open ? "OPEN" : "SECURED") + "\n";
    meta += "BSSID " + n.bssid;
    makeText(content_, meta.c_str(), DIM, display.width() - 16);
    makeAction("USE AS LAB TARGET", safeLabEvent, nullptr, RED);
    makeAction("CONNECT WIFI", connectWifiEvent, nullptr, BLUE);
}

void AppUi::buildWifiConnect() {
    makeText(content_, "WIFI CONNECT", BLUE, display.width() - 16);
    makeText(content_, selectedSsid_.isEmpty() ? "Select an AP first" : selectedSsid_.c_str(), WHITE, display.width() - 16);
    makeAction("CHOOSE NETWORK", scanEvent, nullptr, CYAN);
    makeField("PASSWORD", SoftKeyboard::ALPHA, &wifiPasswordField_, true);
    makeAction("CONNECT", wifiConnectEvent, nullptr, GREEN);
    makeAction("DISCONNECT", wifiDisconnectEvent, nullptr, RED);
    wifiStatus_ = makeText(content_, "READY", DIM, display.width() - 16);
}

void AppUi::buildPacketMonitor() {
    makeText(content_, "PACKET MONITOR", GREEN, display.width() - 16);
    makeText(content_, "PASSIVE CHANNEL MONITOR / NO INJECTION", DIM, display.width() - 16);
    packetInfo_ = makeText(content_, "Waiting for survey...", WHITE, display.width() - 16);

    lv_obj_t *graph = lv_obj_create(content_);
    stylePanel(graph, BLACK, LINE);
    lv_obj_set_width(graph, LV_PCT(100));
    lv_obj_set_height(graph, 190);
    lv_obj_set_style_pad_left(graph, 7, 0);
    lv_obj_set_style_pad_right(graph, 7, 0);
    lv_obj_set_style_pad_top(graph, 8, 0);
    lv_obj_set_style_pad_bottom(graph, 8, 0);
    lv_obj_set_style_pad_column(graph, 2, 0);
    lv_obj_set_flex_flow(graph, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(graph, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);

    for (int ch = 1; ch <= 13; ++ch) {
        lv_obj_t *slot = lv_obj_create(graph);
        stylePanel(slot, PANEL, PANEL);
        lv_obj_set_width(slot, 18);
        lv_obj_set_height(slot, 150);
        lv_obj_set_style_pad_all(slot, 2, 0);
        lv_obj_clear_flag(slot, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *bar = lv_obj_create(slot);
        resetObj(bar);
        lv_obj_set_width(bar, 12);
        lv_obj_set_height(bar, 10);
        lv_obj_set_style_bg_color(bar, lv_color_hex(CYAN), 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_pos(bar, 1, 120);
        packetBars_[ch] = bar;

        lv_obj_t *lab = lv_label_create(slot);
        char chbuf[4]; snprintf(chbuf, sizeof(chbuf), "%d", ch);
        lv_label_set_text(lab, chbuf);
        lv_obj_set_style_text_color(lab, lv_color_hex(DIM), 0);
        lv_obj_align(lab, LV_ALIGN_BOTTOM_MID, 0, 0);
        packetBarLabels_[ch] = lab;
    }

    makeAction("REFRESH SURVEY", scanEvent, nullptr, GREEN);
    renderPacketGraph();
}

void AppUi::buildSettings() {
    makeText(content_, "SETTINGS", AMBER, display.width() - 16);
    makeText(content_, "SYSTEM / DISPLAY / ENVIRONMENT / CLOCK", DIM, display.width() - 16);

    lv_obj_t *blTitle = makeText(content_, "BRIGHTNESS", AMBER, display.width() - 16);
    (void)blTitle;
    brightnessSlider_ = lv_slider_create(content_);
    lv_obj_set_width(brightnessSlider_, LV_PCT(100));
    lv_obj_set_height(brightnessSlider_, 24);
    lv_slider_set_range(brightnessSlider_, 5, 100);
    lv_slider_set_value(brightnessSlider_, settingsStore.brightness(), LV_ANIM_OFF);
    lv_obj_add_event_cb(brightnessSlider_, brightnessEvent, LV_EVENT_VALUE_CHANGED, nullptr);

    makeAction("ROTATE 0° / PORTRAIT", rotationEvent, reinterpret_cast<void *>(static_cast<intptr_t>(0)), WHITE);
    makeAction("ROTATE 90° / LANDSCAPE", rotationEvent, reinterpret_cast<void *>(static_cast<intptr_t>(1)), WHITE);
    makeAction("ROTATE 180° / INVERT", rotationEvent, reinterpret_cast<void *>(static_cast<intptr_t>(2)), WHITE);
    makeAction("ROTATE 270° / LANDSCAPE", rotationEvent, reinterpret_cast<void *>(static_cast<intptr_t>(3)), WHITE);

    settingsInfo_ = makeText(content_, "", WHITE, display.width() - 16);
    makeAction("SYNC CLOCK / NTP", ntpEvent, nullptr, CYAN);
}

void AppUi::buildAbout() {
    makeText(content_, "ABOUT", MAGENTA, display.width() - 16);
    aboutInfo_ = makeText(content_,
        "SALARINI FIRMWARE\nCreated by Salar\n\nIf you copy or fork this firmware, keep credit to Salar.\n\nHardware: ST7796 / FT6336 / SHTC3 / QMI8658 / PCF85063 / AXP2101 / SD.\n\nFuture modules: RF / nRF24 / BLE / USB / OTA.",
        WHITE, display.width() - 16);
    lv_obj_set_height(aboutInfo_, LV_SIZE_CONTENT);
}

void AppUi::refreshHeader() {
    if (!ready_) return;
    if (clock_) {
        String t = timeManager.formatted();
        if (t.length() > 8) t = t.substring(0, 8);
        lv_label_set_text(clock_, t.c_str());
    }
    if (battery_) {
        char buf[24]; snprintf(buf, sizeof(buf), "BAT %u%%", g_state.batteryPercent);
        lv_label_set_text(battery_, buf);
        lv_obj_set_style_text_color(battery_, lv_color_hex(g_state.charging ? AMBER : GREEN), 0);
    }
}

String AppUi::scanSummary() const {
    return String(wifiManager.scanCount()) + " APs  /  " + wifiManager.scanStatus();
}

String AppUi::channelSummary() const {
    int counts[14]{};
    const int total = min(wifiManager.scanCount(), 40);
    for (int i = 0; i < total; ++i) {
        const WifiNetworkInfo n = wifiManager.network(i);
        if (n.channel >= 1 && n.channel <= 13) counts[n.channel]++;
    }
    String s;
    for (int ch = 1; ch <= 13; ++ch) {
        if (counts[ch]) {
            if (s.length()) s += "  ";
            s += String(ch) + ":" + String(counts[ch]);
        }
    }
    return s.length() ? s : String("no channel data");
}

String AppUi::sensorSummary() const {
    String s;
    s += "AMBIENT  " + String(g_state.temperatureC, 1) + " C\n";
    s += "HUMIDITY " + String(g_state.humidityPct, 1) + " %\n";
    s += "IMU TEMP " + String(g_state.imuTempC, 1) + " C\n";
    s += "ACCEL    " + String(g_state.accelX, 2) + "," + String(g_state.accelY, 2) + "," + String(g_state.accelZ, 2) + "\n";
    s += "GYRO     " + String(g_state.gyroX, 1) + "," + String(g_state.gyroY, 1) + "," + String(g_state.gyroZ, 1) + "\n";
    s += "BAT      " + String(g_state.batteryPercent) + "%  " + String(g_state.batteryMv) + " mV\n";
    s += "VBUS     " + String(g_state.vbusMv) + " mV\n";
    s += "SYSTEM   " + String(g_state.systemMv) + " mV\n";
    s += "PMU TEMP " + String(g_state.pmuTempC, 1) + " C\n";
    s += "RTC      " + String(g_state.rtcOk ? "OK" : "OFF") + "\n";
    s += "SD       " + String(g_state.sdOk ? "OK" : "OFF");
    return s;
}

void AppUi::renderScanResults() {
    if (!scanList_) return;
    lv_obj_clean(scanList_);
    const int count = wifiManager.scanCount();
    lastScanCount_ = count;
    if (count <= 0) {
        makeText(scanList_, wifiManager.scanInProgress() ? "Scanning..." : "No APs found", DIM, display.width() - 32);
        return;
    }

    const int limit = min(count, 40);
    for (int i = 0; i < limit; ++i) {
        const WifiNetworkInfo n = wifiManager.network(i);
        lv_obj_t *b = lv_btn_create(scanList_);
        styleButton(b, n.open ? AMBER : CYAN);
        lv_obj_set_width(b, LV_PCT(100));
        lv_obj_set_height(b, 46);
        lv_obj_add_event_cb(b, wifiSelectEvent, LV_EVENT_CLICKED, reinterpret_cast<void *>(static_cast<intptr_t>(i)));

        String ssid = n.ssid.length() ? n.ssid : String("<hidden>");
        String lower = String(n.rssi) + " dBm / CH" + String(n.channel) + (n.open ? " / OPEN" : " / SEC");
        String detail = ssid + "\n" + lower;
        lv_obj_t *lab = lv_label_create(b);
        lv_label_set_text(lab, detail.c_str());
        lv_obj_set_style_text_color(lab, lv_color_hex(WHITE), 0);
        lv_obj_set_width(lab, LV_PCT(100));
        lv_label_set_long_mode(lab, LV_LABEL_LONG_CLIP);
        lv_obj_center(lab);
    }
}

void AppUi::renderPacketGraph() {
    if (!packetInfo_) return;
    String info = "APs " + String(wifiManager.scanCount());
    info += "   CH " + channelSummary();
    if (g_state.wifiConnected) info += "\nCONNECTED " + String(g_state.wifiRssi) + " dBm";
    lv_label_set_text(packetInfo_, info.c_str());

    int counts[14]{};
    const int total = min(wifiManager.scanCount(), 40);
    for (int i = 0; i < total; ++i) {
        const WifiNetworkInfo n = wifiManager.network(i);
        if (n.channel >= 1 && n.channel <= 13) counts[n.channel]++;
    }
    int maxCount = 1;
    for (int ch = 1; ch <= 13; ++ch) maxCount = max(maxCount, counts[ch]);

    for (int ch = 1; ch <= 13; ++ch) {
        if (!packetBars_[ch]) continue;
        const int h = 10 + (110 * counts[ch]) / maxCount;
        lv_obj_set_height(packetBars_[ch], h);
        lv_obj_set_y(packetBars_[ch], 122 - h + 10);
        const uint32_t c = counts[ch] ? (counts[ch] >= 3 ? AMBER : CYAN) : LINE;
        lv_obj_set_style_bg_color(packetBars_[ch], lv_color_hex(c), 0);
    }
}

void AppUi::refreshPage() {
    if (!ready_) return;
    const uint32_t now = millis();

    if (view_ == VIEW_SCAN) {
        String s = scanSummary();
        if (wifiManager.scanInProgress()) {
            static const char frames[] = "|/-\\";
            s = String("SCANNING ") + frames[(now / 160U) & 3];
        }
        s += "\nCH " + channelSummary();
        if (scanInfo_) lv_label_set_text(scanInfo_, s.c_str());
        if (!wifiManager.scanInProgress() && wifiManager.scanCount() != lastScanCount_) renderScanResults();
    } else if (view_ == VIEW_SETTINGS && settingsInfo_) {
        String s = sensorSummary();
        s += "\nTIME " + timeManager.formatted();
        s += "\nROT  " + String(settingsStore.rotation() * 90) + "°";
        lv_label_set_text(settingsInfo_, s.c_str());
    } else if (view_ == VIEW_PACKET_MONITOR) {
        renderPacketGraph();
        if (wifiManager.scanInProgress()) {
            static const char frames[] = "|/-\\";
            if (packetInfo_) {
                String s = "SURVEY " + String(frames[(now / 160U) & 3]);
                lv_label_set_text(packetInfo_, s.c_str());
            }
        }
    } else if (view_ == VIEW_WIFI_CONNECT && wifiStatus_) {
        if (g_state.wifiConnected) {
            String s = String("CONNECTED  ") + g_state.wifiIp;
            lv_label_set_text(wifiStatus_, s.c_str());
        } else if (g_state.wifiConnecting) {
            lv_label_set_text(wifiStatus_, "CONNECTING...");
        } else {
            lv_label_set_text(wifiStatus_, "READY");
        }
    } else if (labAnimUntil_ > now && status_) {
        static const char frames[] = ".oO0Oo.";
        char buf[64];
        snprintf(buf, sizeof(buf), "LAB SIMULATION %c", frames[(now / 140U) % (sizeof(frames) - 1)]);
        lv_label_set_text(status_, buf);
    }

    if (view_ == VIEW_PACKET_MONITOR && packetNextScanMs_ <= now && !wifiManager.scanInProgress()) {
        packetNextScanMs_ = now + 9000UL;
        wifiManager.startScan();
    }
}

void AppUi::update() {
    if (!ready_) return;
    refreshHeader();
    refreshPage();
}

void AppUi::openKeyboard(lv_obj_t *target, SoftKeyboard::Mode mode) {
    if (!target || !root_) return;
    softKeyboard.open(root_, target, mode);
    setContentHeightForKeyboard(true);
    lv_obj_scroll_to_view_recursive(target, LV_ANIM_OFF);
}

void AppUi::rebuildForRotation() {
    if (!root_ || !content_ || !header_) return;
    lv_obj_set_width(root_, display.width());
    lv_obj_set_height(root_, display.height());
    lv_obj_set_width(header_, display.width());
    lv_obj_set_height(header_, HEADER_H);
    lv_obj_set_width(content_, display.width());
    lv_obj_set_height(content_, max(80, static_cast<int>(display.height()) - HEADER_H));
    lv_obj_set_pos(content_, 0, HEADER_H);
    show(view_, false);
}

void AppUi::setContentHeightForKeyboard(bool visible) {
    if (!content_) return;
    int h = static_cast<int>(display.height()) - HEADER_H;
    if (visible) h = max(70, h - 250);
    lv_obj_set_height(content_, h);
}

void AppUi::setStatus(const String &s) {
    if (status_) lv_label_set_text(status_, s.c_str());
}

void AppUi::animatePageEntry() {
    // Keep page transitions cheap: animate only the title text, never the
    // whole content tree. Whole-tree opacity can force costly blending layers.
    if (!pageTitle_) return;
    const char *base = lv_label_get_text(pageTitle_);
    if (!base) return;
    char buf[96];
    snprintf(buf, sizeof(buf), "> %s", base);
    lv_label_set_text(pageTitle_, buf);
}

void AppUi::navEvent(lv_event_t *e) {
    const View v = static_cast<View>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    ui.show(v, true);
    g_state.uiInteractions++;
}

void AppUi::backEvent(lv_event_t *) {
    if (ui.view_ == VIEW_ROOT) return;
    View v = VIEW_ROOT;
    switch (ui.view_) {
        case VIEW_EXPLOIT: case VIEW_SCAN: case VIEW_PACKET_MONITOR: case VIEW_SETTINGS: case VIEW_ABOUT: case VIEW_WIFI_CONNECT: v = VIEW_ROOT; break;
        case VIEW_DEAUTH: case VIEW_EVIL_TWIN: case VIEW_COOL_STUFF: case VIEW_BEACON: case VIEW_AP_SPOOF: case VIEW_BT_ADV: v = VIEW_EXPLOIT; break;
        case VIEW_APS: case VIEW_STAS: v = VIEW_DEAUTH; break;
        case VIEW_AP_DETAIL: v = VIEW_SCAN; break;
        default: v = ui.previousView_; break;
    }
    ui.show(v, false);
    g_state.uiInteractions++;
}

void AppUi::scanEvent(lv_event_t *) {
    ui.selectingForConnect_ = (ui.view_ == VIEW_WIFI_CONNECT);
    ui.scanAnimUntil_ = millis() + 1800UL;
    ui.lastScanCount_ = -1;
    wifiManager.startScan();
    ui.show(VIEW_SCAN, true);
    g_state.uiInteractions++;
}

void AppUi::wifiSelectEvent(lv_event_t *e) {
    const int idx = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    if (idx < 0 || idx >= wifiManager.scanCount()) return;
    const WifiNetworkInfo n = wifiManager.network(idx);
    ui.selectedWifiIndex_ = idx;
    ui.selectedSsid_ = n.ssid;
    if (ui.selectingForConnect_) {
        ui.selectingForConnect_ = false;
        ui.show(VIEW_WIFI_CONNECT, true);
    } else {
        ui.show(VIEW_AP_DETAIL, true);
    }
    g_state.uiInteractions++;
}

void AppUi::connectWifiEvent(lv_event_t *) {
    ui.show(VIEW_WIFI_CONNECT, true);
}

void AppUi::wifiConnectEvent(lv_event_t *) {
    if (ui.selectedSsid_.isEmpty()) {
        ui.setStatus("SELECT NETWORK FIRST");
        return;
    }
    const char *pwd = ui.wifiPasswordField_ ? lv_textarea_get_text(ui.wifiPasswordField_) : "";
    wifiManager.connect(ui.selectedSsid_, String(pwd));
    ui.setStatus("CONNECTING...");
    g_state.uiInteractions++;
}

void AppUi::wifiDisconnectEvent(lv_event_t *) {
    wifiManager.disconnect();
    ui.setStatus("DISCONNECTED");
    g_state.uiInteractions++;
}

void AppUi::openInputEvent(lv_event_t *e) {
    const auto mode = static_cast<SoftKeyboard::Mode>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    ui.openKeyboard(lv_event_get_target(e), mode);
    g_state.uiInteractions++;
}

void AppUi::brightnessEvent(lv_event_t *e) {
    const int v = lv_slider_get_value(lv_event_get_target(e));
    settingsStore.setBrightness(static_cast<uint8_t>(v));
    boardIO.setBacklight(static_cast<uint8_t>(v));
    g_state.uiInteractions++;
}

void AppUi::rotationEvent(lv_event_t *e) {
    const uint8_t r = static_cast<uint8_t>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)) & 0x03);
    settingsStore.setRotation(r);
    if (display.setRotation(r)) ui.rebuildForRotation();
    g_state.uiInteractions++;
}

void AppUi::ntpEvent(lv_event_t *) {
    ui.setStatus(timeManager.syncNtp() ? "NTP SYNCED" : "NTP FAILED");
    g_state.uiInteractions++;
}

void AppUi::safeLabEvent(lv_event_t *) {
    ui.labAnimUntil_ = millis() + 2200UL;
    if (!ui.status_) {
        ui.status_ = ui.makeText(ui.content_, "", MAGENTA, display.width() - 16);
    }
    ui.setStatus("LAB SIMULATION READY");
    g_state.uiInteractions++;
}

void AppUi::timerCb(lv_timer_t *) {
    ui.uiPhase_++;
    if (ui.scanAnimUntil_ > millis() && ui.scanInfo_) lv_obj_invalidate(ui.scanInfo_);
    if (ui.labAnimUntil_ > millis() && ui.status_) lv_obj_invalidate(ui.status_);
}

void AppUi::keyboardVisibility(bool visible) {
    ui.setContentHeightForKeyboard(visible);
    if (!visible) {
        if (ui.content_) lv_obj_set_height(ui.content_, max(80, static_cast<int>(display.height()) - HEADER_H));
    }
}
