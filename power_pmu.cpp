#include "power_pmu.h"
#include "config.h"
#include <Wire.h>
#define XPOWERS_CHIP_AXP2101
#include <XPowersLib.h>

namespace {
XPowersPMU power;

// AXP2101 exposes a percentage register, but on some board revisions that
// value can remain at a rail value (commonly 100) even while the battery ADC
// changes. Use the measured single-cell Li-ion voltage as the displayed
// percentage so the UI follows the actual battery voltage.
uint8_t voltageToPercent(uint16_t mv) {
    if (mv == 0) return 0;
    static const uint16_t v[] = {3300, 3400, 3500, 3600, 3700, 3800, 3900, 4000, 4100, 4200};
    static const uint8_t  p[] = {0,    5,    12,   25,   40,   55,   68,   80,   91,   100};
    if (mv <= v[0]) return 0;
    if (mv >= v[9]) return 100;
    for (size_t i = 1; i < 10; ++i) {
        if (mv <= v[i]) {
            const uint32_t dv = v[i] - v[i - 1];
            const uint32_t dp = p[i] - p[i - 1];
            return static_cast<uint8_t>(p[i - 1] + ((static_cast<uint32_t>(mv - v[i - 1]) * dp + dv / 2U) / dv));
        }
    }
    return 0;
}
}

PowerPmu pmu;

bool PowerPmu::begin() {
    if (!power.begin(Wire, C5Config::AXP2101_ADDR, C5Config::I2C_SDA, C5Config::I2C_SCL)) {
        Serial.println("AXP2101 not found at 0x34");
        return false;
    }

    power.disableTSPinMeasure();
    power.enableBattDetection();
    power.enableVbusVoltageMeasure();
    power.enableBattVoltageMeasure();
    power.enableSystemVoltageMeasure();
    power.enableTemperatureMeasure();

    // Keep the board-safe charging configuration from the supplied example.
    power.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_50MA);
    power.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_400MA);
    power.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);
    power.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);

    power.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    power.clearIrqStatus();
    power.enableIRQ(
        XPOWERS_AXP2101_BAT_INSERT_IRQ |
        XPOWERS_AXP2101_BAT_REMOVE_IRQ |
        XPOWERS_AXP2101_VBUS_INSERT_IRQ |
        XPOWERS_AXP2101_VBUS_REMOVE_IRQ |
        XPOWERS_AXP2101_PKEY_SHORT_IRQ |
        XPOWERS_AXP2101_PKEY_LONG_IRQ |
        XPOWERS_AXP2101_BAT_CHG_DONE_IRQ |
        XPOWERS_AXP2101_BAT_CHG_START_IRQ);

    ready_ = true;
    Serial.printf("AXP2101 ready, chip ID 0x%02X\n", power.getChipID());
    update();
    return true;
}

bool PowerPmu::update() {
    if (!ready_) return false;
    batteryConnected_ = power.isBatteryConnect();
    charging_ = power.isCharging();
    discharging_ = power.isDischarge();
    batteryMv_ = power.getBattVoltage();
    vbusMv_ = power.getVbusVoltage();
    systemMv_ = power.getSystemVoltage();
    temperatureC_ = power.getTemperature();
    if (!batteryConnected_ || batteryMv_ == 0) {
        batteryPercent_ = 0;
    } else {
        const uint8_t voltagePct = voltageToPercent(batteryMv_);
        const int gaugePct = power.getBatteryPercent();
        // Treat obviously stuck/invalid gauge values as unusable. Otherwise
        // voltage remains the displayed source because it tracks the ADC.
        (void)gaugePct;
        batteryPercent_ = voltagePct;
    }
    return true;
}

void PowerPmu::pollIrqEvents() {
    if (!ready_) return;
    power.getIrqStatus();
    if (power.isVbusInsertIrq()) Serial.println("PMU: VBUS inserted");
    if (power.isVbusRemoveIrq()) Serial.println("PMU: VBUS removed");
    if (power.isBatInsertIrq()) Serial.println("PMU: battery inserted");
    if (power.isBatRemoveIrq()) Serial.println("PMU: battery removed");
    if (power.isBatChargeStartIrq()) Serial.println("PMU: charging started");
    if (power.isBatChargeDoneIrq()) Serial.println("PMU: charging complete");
    if (power.isPekeyShortPressIrq()) Serial.println("PMU: power-key short press");
    if (power.isPekeyLongPressIrq()) Serial.println("PMU: power-key long press");
    power.clearIrqStatus();
}

void PowerPmu::printRails() {
    if (!ready_) return;
    Serial.println("AXP2101 DCDC ------------------------------------------------");
    Serial.printf("DC1     %s  %u mV\n", power.isEnableDC1() ? "ON " : "OFF", power.getDC1Voltage());
    Serial.printf("DC2     %s  %u mV\n", power.isEnableDC2() ? "ON " : "OFF", power.getDC2Voltage());
    Serial.printf("DC3     %s  %u mV\n", power.isEnableDC3() ? "ON " : "OFF", power.getDC3Voltage());
    Serial.printf("DC4     %s  %u mV\n", power.isEnableDC4() ? "ON " : "OFF", power.getDC4Voltage());
    Serial.printf("DC5     %s  %u mV\n", power.isEnableDC5() ? "ON " : "OFF", power.getDC5Voltage());
    Serial.println("AXP2101 LDO -------------------------------------------------");
    Serial.printf("ALDO1   %s  %u mV\n", power.isEnableALDO1() ? "ON " : "OFF", power.getALDO1Voltage());
    Serial.printf("ALDO2   %s  %u mV\n", power.isEnableALDO2() ? "ON " : "OFF", power.getALDO2Voltage());
    Serial.printf("ALDO3   %s  %u mV\n", power.isEnableALDO3() ? "ON " : "OFF", power.getALDO3Voltage());
    Serial.printf("ALDO4   %s  %u mV\n", power.isEnableALDO4() ? "ON " : "OFF", power.getALDO4Voltage());
    Serial.printf("BLDO1   %s  %u mV\n", power.isEnableBLDO1() ? "ON " : "OFF", power.getBLDO1Voltage());
    Serial.printf("BLDO2   %s  %u mV\n", power.isEnableBLDO2() ? "ON " : "OFF", power.getBLDO2Voltage());
    Serial.printf("CPUSLDO %s  %u mV\n", power.isEnableCPUSLDO() ? "ON " : "OFF", power.getCPUSLDOVoltage());
    Serial.printf("DLDO1   %s  %u mV\n", power.isEnableDLDO1() ? "ON " : "OFF", power.getDLDO1Voltage());
    Serial.printf("DLDO2   %s  %u mV\n", power.isEnableDLDO2() ? "ON " : "OFF", power.getDLDO2Voltage());
    Serial.println("--------------------------------------------------------------");
}
