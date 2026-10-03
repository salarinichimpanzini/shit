#include "sd_storage.h"
#include "config.h"

#include <stdio.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <driver/sdspi_host.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>

SdStorage sdStorage;

bool SdStorage::beginOnSharedSpiBus() {
    if (ready_) return true;

    // LCD and SD share SPI2. Keep LCD CS high while the card is mounted.
    pinMode(C5Config::LCD_CS, OUTPUT);
    digitalWrite(C5Config::LCD_CS, HIGH);
    pinMode(C5Config::SD_CS, OUTPUT);
    digitalWrite(C5Config::SD_CS, HIGH);

    spi_bus_config_t bus_cfg{};
    bus_cfg.mosi_io_num = C5Config::SD_MOSI;
    bus_cfg.miso_io_num = C5Config::SD_MISO;
    bus_cfg.sclk_io_num = C5Config::SD_CLK;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.max_transfer_sz = static_cast<int>(C5Config::LCD_MAX_TRANSFER_BYTES);

    esp_err_t err = spi_bus_initialize(SPI2_HOST, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        Serial.printf("SD/SPI2 bus init failed: %s\n", esp_err_to_name(err));
        return false;
    }

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg{};
    mount_cfg.format_if_mount_failed = C5Config::SD_FORMAT_ON_FAIL;
    mount_cfg.max_files = 8;
    mount_cfg.allocation_unit_size = 512;
    mount_cfg.disk_status_check_enable = true;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;
    host.max_freq_khz = C5Config::SD_FREQ_KHZ;

    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.gpio_cs = static_cast<gpio_num_t>(C5Config::SD_CS);
    slot_cfg.host_id = SPI2_HOST;

    sdmmc_card_t *card = nullptr;
    err = esp_vfs_fat_sdspi_mount(C5Config::SD_MOUNT, &host, &slot_cfg, &mount_cfg, &card);
    if (err != ESP_OK || card == nullptr) {
        Serial.printf("SD mount failed: %s\n", esp_err_to_name(err));
        return false;
    }

    card_ = card;
    capacityGb_ = static_cast<float>(card->csd.capacity) / 2048.0f / 1024.0f;
    ready_ = true;
    sdmmc_card_print_info(stdout, card);
    Serial.printf("SD mounted at %s, %.2f GB\n", C5Config::SD_MOUNT, capacityGb_);
    ensureLogFile();
    return true;
}

bool SdStorage::exists(const char *path) const {
    if (!ready_ || !path) return false;
    struct stat st{};
    return stat(path, &st) == 0;
}

esp_err_t SdStorage::writeText(const char *path, const char *text) {
    if (!ready_ || !path || !text) return ESP_ERR_INVALID_ARG;
    sdmmc_card_t *card = static_cast<sdmmc_card_t *>(card_);
    if (!card) return ESP_ERR_NOT_FOUND;
    esp_err_t status = sdmmc_get_status(card);
    if (status != ESP_OK) return status;
    FILE *f = fopen(path, "w");
    if (!f) return ESP_ERR_NOT_FOUND;
    fputs(text, f);
    fclose(f);
    return ESP_OK;
}

esp_err_t SdStorage::appendText(const char *path, const char *text) {
    if (!ready_ || !path || !text) return ESP_ERR_INVALID_ARG;
    sdmmc_card_t *card = static_cast<sdmmc_card_t *>(card_);
    if (!card) return ESP_ERR_NOT_FOUND;
    esp_err_t status = sdmmc_get_status(card);
    if (status != ESP_OK) return status;
    FILE *f = fopen(path, "a");
    if (!f) return ESP_ERR_NOT_FOUND;
    fputs(text, f);
    fclose(f);
    return ESP_OK;
}

bool SdStorage::ensureLogFile() {
    if (!ready_) return false;
    if (exists(C5Config::SD_LOG_FILE)) return true;
    return writeText(C5Config::SD_LOG_FILE,
        "timestamp,temperature_c,humidity_pct,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,battery_pct,battery_mv,vbus_mv\n") == ESP_OK;
}
