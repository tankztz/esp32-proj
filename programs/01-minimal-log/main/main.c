#include <stdint.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "minimal_log";

void app_main(void)
{
    uint32_t counter = 0;

    ESP_LOGI(TAG, "minimal ESP32 app started");
    ESP_LOGI(TAG, "board notes: ESP32-D0WDQ6, CP210x UART, COM20");

    while (true) {
        ESP_LOGI(TAG, "alive %lu", (unsigned long)counter++);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
