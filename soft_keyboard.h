#pragma once

#include <Arduino.h>
#define LV_CONF_INCLUDE_SIMPLE
#include <lvgl.h>

class SoftKeyboard {
public:
    enum Mode : uint8_t { ALPHA, NUMERIC, SYMBOLS };
    using VisibilityCallback = void (*)(bool visible);

    void setVisibilityCallback(VisibilityCallback cb) { visibilityCallback_ = cb; }
    void open(lv_obj_t *parent, lv_obj_t *textarea, Mode mode);
    void close();
    bool visible() const { return panel_ != nullptr; }
    Mode mode() const { return mode_; }

private:
    lv_obj_t *panel_ = nullptr;
    lv_obj_t *title_ = nullptr;
    lv_obj_t *target_ = nullptr;
    Mode mode_ = ALPHA;
    bool upper_ = false;
    VisibilityCallback visibilityCallback_ = nullptr;

    void rebuild();
    void addKey(const char *label, int x, int y, int w, int h, uint8_t action = 0);
    void handleKey(const String &key);
    static void keyEvent(lv_event_t *e);
};

extern SoftKeyboard softKeyboard;
