#include "sensor_imu.h"
#include "config.h"
#include <Arduino.h>
#include <Wire.h>

ImuSensor imu;

bool ImuSensor::begin() {
    ready_ = imu_.begin(Wire, C5Config::QMI8658_ADDR, C5Config::I2C_SDA, C5Config::I2C_SCL);
    if (!ready_) {
        Serial.println("QMI8658 not found; trying the alternate I2C address 0x6A");
        ready_ = imu_.begin(Wire, 0x6A, C5Config::I2C_SDA, C5Config::I2C_SCL);
    }
    if (!ready_) return false;

    chipId_ = imu_.getChipID();
    imu_.configAccelerometer(SensorQMI8658::ACC_RANGE_8G, SensorQMI8658::ACC_ODR_125Hz, SensorQMI8658::LPF_MODE_2);
    imu_.configGyroscope(SensorQMI8658::GYR_RANGE_512DPS, SensorQMI8658::GYR_ODR_112_1Hz, SensorQMI8658::LPF_MODE_2);
    imu_.enableAccelerometer();
    imu_.enableGyroscope();
    imu_.disablePedometer();
    Serial.printf("QMI8658 ready, chip ID 0x%02X\n", chipId_);
    return true;
}

bool ImuSensor::update() {
    if (!ready_) return false;
    const bool accOk = imu_.getAccelerometer(ax_, ay_, az_);
    const bool gyroOk = imu_.getGyroscope(gx_, gy_, gz_) != 0;
    if (!accOk || !gyroOk) return false;
    temperatureC_ = imu_.getTemperature_C();
    return true;
}
