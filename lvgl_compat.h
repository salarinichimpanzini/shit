#pragma once

#include <lvgl.h>

/* SALARINI TOOL targets LVGL 8.x. Widget-specific parts are enum values, not macros. */
#if defined(LVGL_VERSION_MAJOR) && (LVGL_VERSION_MAJOR == 8)
  constexpr int SALARINI_LVGL_TEXTAREA_PLACEHOLDER_PART = LV_PART_TEXTAREA_PLACEHOLDER;
#endif
