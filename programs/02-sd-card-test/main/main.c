#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>

#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdmmc_cmd.h"

#define MOUNT_POINT "/sdcard"

#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   4

static const char *TAG = "sd_card_test";

static void list_root_directory(void)
{
    DIR *dir = opendir(MOUNT_POINT);
    if (dir == NULL) {
        ESP_LOGE(TAG, "failed to open %s", MOUNT_POINT);
        return;
    }

    ESP_LOGI(TAG, "root directory:");
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char path[320];
        struct stat st;
        snprintf(path, sizeof(path), "%s/%s", MOUNT_POINT, entry->d_name);
        if (stat(path, &st) == 0) {
            ESP_LOGI(TAG, "  %s%s (%ld bytes)",
                     entry->d_name,
                     S_ISDIR(st.st_mode) ? "/" : "",
                     (long)st.st_size);
        } else {
            ESP_LOGI(TAG, "  %s", entry->d_name);
        }
    }

    closedir(dir);
}

void app_main(void)
{
    ESP_LOGI(TAG, "SPI SD card test starting");
    ESP_LOGI(TAG, "pins: CLK=%d MOSI=%d MISO=%d CS=%d",
             PIN_NUM_CLK, PIN_NUM_MOSI, PIN_NUM_MISO, PIN_NUM_CS);

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 4,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = 400;

    gpio_set_pull_mode(PIN_NUM_MISO, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(PIN_NUM_MOSI, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(PIN_NUM_CLK, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(PIN_NUM_CS, GPIO_PULLUP_ONLY);

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
        return;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    sdmmc_card_t *card = NULL;
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD mount failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "check card insertion and pins; this test assumes CS=4 MISO=19");
        spi_bus_free(host.slot);
        return;
    }

    ESP_LOGI(TAG, "SD card mounted at %s", MOUNT_POINT);
    sdmmc_card_print_info(stdout, card);
    list_root_directory();

    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    spi_bus_free(host.slot);
    ESP_LOGI(TAG, "SD card unmounted");

    while (true) {
        ESP_LOGI(TAG, "done");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
