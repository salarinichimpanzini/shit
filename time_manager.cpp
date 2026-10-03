#include "time_manager.h"

#include <time.h>
#include <WiFi.h>
#include <sys/time.h>
#include "config.h"
#include "app_state.h"
#include "settings.h"
#include "sensor_rtc.h"

TimeManager timeManager;

static bool saneTm(const struct tm &t) {
    return t.tm_year + 1900 >= 2020 && t.tm_mon >= 0 && t.tm_mon < 12 && t.tm_mday >= 1 && t.tm_mday <= 31;
}

bool TimeManager::begin() {
    setenv("TZ", C5Config::TZ_RULE, 1);
    tzset();

    const uint32_t saved = settingsStore.lastEpoch();
    if (saved >= 1700000000UL) {
        struct timeval tv{};
        tv.tv_sec = static_cast<time_t>(saved);
        settimeofday(&tv, nullptr);
        synced_ = false;
    }

    refreshState();
    if (rtcSensor.ready()) {
        const RtcDateTime &dt = rtcSensor.dateTime();
        if (dt.year >= 2020) {
            struct tm local{};
            local.tm_year = dt.year - 1900;
            local.tm_mon = dt.month - 1;
            local.tm_mday = dt.day;
            local.tm_hour = dt.hour;
            local.tm_min = dt.minute;
            local.tm_sec = dt.second;
            local.tm_isdst = -1;
            const time_t epoch = mktime(&local);
            if (epoch > 1700000000) {
                struct timeval tv{};
                tv.tv_sec = epoch;
                settimeofday(&tv, nullptr);
                settingsStore.setLastEpoch(static_cast<uint32_t>(epoch));
                refreshState();
            }
        }
    }
    return valid_;
}

void TimeManager::refreshState() {
    time_t now = time(nullptr);
    valid_ = now >= 1700000000;
    if (!valid_) return;
    struct tm local{};
    localtime_r(&now, &local);
    g_state.year = static_cast<uint16_t>(local.tm_year + 1900);
    g_state.month = static_cast<uint8_t>(local.tm_mon + 1);
    g_state.day = static_cast<uint8_t>(local.tm_mday);
    g_state.hour = static_cast<uint8_t>(local.tm_hour);
    g_state.minute = static_cast<uint8_t>(local.tm_min);
    g_state.second = static_cast<uint8_t>(local.tm_sec);
    g_state.week = static_cast<uint8_t>(local.tm_wday);
    g_state.timeSynced = synced_;
}

void TimeManager::service(bool wifiConnected) {
    refreshState();

    if (wifiConnected && !synced_ && !syncInProgress_ && millis() - lastSyncAttemptMs_ > 15000UL) {
        syncNtp();
    }
}

bool TimeManager::syncNtp() {
    if (WiFi.status() != WL_CONNECTED) {
        lastError_ = "WiFi offline";
        return false;
    }

    syncInProgress_ = true;
    lastSyncAttemptMs_ = millis();
    configTzTime(C5Config::TZ_RULE, C5Config::NTP_1, C5Config::NTP_2, C5Config::NTP_3);

    struct tm info{};
    const bool ok = getLocalTime(&info, 700);
    if (ok && saneTm(info)) {
        const time_t epoch = mktime(&info);
        if (epoch > 1700000000) {
            settingsStore.setLastEpoch(static_cast<uint32_t>(epoch));
            rtcSensor.setDateTime(
                static_cast<uint16_t>(info.tm_year + 1900),
                static_cast<uint8_t>(info.tm_mon + 1),
                static_cast<uint8_t>(info.tm_mday),
                static_cast<uint8_t>(info.tm_hour),
                static_cast<uint8_t>(info.tm_min),
                static_cast<uint8_t>(info.tm_sec));
            synced_ = true;
            lastError_.clear();
            refreshState();
            syncInProgress_ = false;
            return true;
        }
    }

    lastError_ = "NTP timeout";
    syncInProgress_ = false;
    refreshState();
    return false;
}

bool TimeManager::setManual(const String &text) {
    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    if (sscanf(text.c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6) {
        if (sscanf(text.c_str(), "%d/%d/%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6) {
            return false;
        }
    }
    if (year < 2020 || year > 2099 || month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
        return false;
    }

    struct tm local{};
    local.tm_year = year - 1900;
    local.tm_mon = month - 1;
    local.tm_mday = day;
    local.tm_hour = hour;
    local.tm_min = minute;
    local.tm_sec = second;
    local.tm_isdst = -1;
    const time_t epoch = mktime(&local);
    if (epoch <= 0) return false;

    struct timeval tv{};
    tv.tv_sec = epoch;
    tv.tv_usec = 0;
    if (settimeofday(&tv, nullptr) != 0) return false;

    rtcSensor.setDateTime(year, month, day, hour, minute, second);
    settingsStore.setLastEpoch(static_cast<uint32_t>(epoch));
    synced_ = false;
    refreshState();
    return true;
}

bool TimeManager::setRtcFromSystemTime() {
    time_t now = time(nullptr);
    if (now <= 0 || !rtcSensor.ready()) return false;
    struct tm local{};
    localtime_r(&now, &local);
    const bool ok = rtcSensor.setDateTime(
        static_cast<uint16_t>(local.tm_year + 1900),
        static_cast<uint8_t>(local.tm_mon + 1),
        static_cast<uint8_t>(local.tm_mday),
        static_cast<uint8_t>(local.tm_hour),
        static_cast<uint8_t>(local.tm_min),
        static_cast<uint8_t>(local.tm_sec));
    if (ok) settingsStore.setLastEpoch(static_cast<uint32_t>(now));
    return ok;
}

String TimeManager::formatted() const {
    if (!valid_) return "--:--:--  ----/--/--";
    char buf[32]{};
    snprintf(buf, sizeof(buf), "%02u:%02u:%02u  %04u-%02u-%02u",
             g_state.hour, g_state.minute, g_state.second,
             g_state.year, g_state.month, g_state.day);
    return String(buf);
}
