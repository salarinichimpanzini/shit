#pragma once

#include "libraries/SensorLib/src/SensorQMI8658.hpp"

class ImuSensor {
public:
    bool begin();
    bool update();
    bool ready() const { return ready_; }
    float ax() const { return ax_; }
    float ay() const { return ay_; }
    float az() const { return az_; }
    float gx() const { return gx_; }
    float gy() const { return gy_; }
    float gz() const { return gz_; }
    float temperatureC() const { return temperatureC_; }
    uint32_t steps() const { return steps_; }
    uint8_t chipId() const { return chipId_; }

private:
    SensorQMI8658 imu_;
    bool ready_ = false;
    uint8_t chipId_ = 0;
    float ax_ = 0, ay_ = 0, az_ = 0;
    float gx_ = 0, gy_ = 0, gz_ = 0;
    float temperatureC_ = 0;
    uint32_t steps_ = 0;
};

extern ImuSensor imu;
