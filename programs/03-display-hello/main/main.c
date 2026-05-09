#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LCD_HOST SPI3_HOST

#define LCD_WIDTH  320
#define LCD_HEIGHT 240

#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   14
#define PIN_NUM_DC   27
#define PIN_NUM_RST  33
#define PIN_NUM_BKLT 32

#define COLOR_BLACK  0x0000
#define COLOR_WHITE  0xffff
#define COLOR_RED    0xf800
#define COLOR_GREEN  0x07e0
#define COLOR_BLUE   0x001f
#define COLOR_YELLOW 0xffe0
#define COLOR_CYAN   0x07ff
#define COLOR_NAVY   0x0011

static const char *TAG = "display_hello";
static spi_device_handle_t s_lcd;

static esp_err_t lcd_send(const uint8_t *data, int len, bool is_data)
{
    spi_transaction_t trans = {
        .length = len * 8,
        .tx_buffer = data,
    };

    gpio_set_level(PIN_NUM_DC, is_data);
    return spi_device_polling_transmit(s_lcd, &trans);
}

static esp_err_t lcd_cmd(uint8_t cmd)
{
    return lcd_send(&cmd, 1, false);
}

static esp_err_t lcd_data(const uint8_t *data, int len)
{
    return lcd_send(data, len, true);
}

static esp_err_t lcd_cmd_data(uint8_t cmd, const uint8_t *data, int len)
{
    ESP_RETURN_ON_ERROR(lcd_cmd(cmd), TAG, "cmd 0x%02x failed", cmd);
    if (len > 0) {
        ESP_RETURN_ON_ERROR(lcd_data(data, len), TAG, "data for 0x%02x failed", cmd);
    }
    return ESP_OK;
}

static void lcd_reset(void)
{
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
}

static esp_err_t lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t data[4];

    data[0] = x0 >> 8;
    data[1] = x0 & 0xff;
    data[2] = x1 >> 8;
    data[3] = x1 & 0xff;
    ESP_RETURN_ON_ERROR(lcd_cmd_data(0x2a, data, sizeof(data)), TAG, "set column failed");

    data[0] = y0 >> 8;
    data[1] = y0 & 0xff;
    data[2] = y1 >> 8;
    data[3] = y1 & 0xff;
    ESP_RETURN_ON_ERROR(lcd_cmd_data(0x2b, data, sizeof(data)), TAG, "set row failed");

    return lcd_cmd(0x2c);
}

static esp_err_t lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint16_t line[LCD_WIDTH];
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xff;

    for (int i = 0; i < w; ++i) {
        line[i] = ((uint16_t)lo << 8) | hi;
    }

    ESP_RETURN_ON_ERROR(lcd_set_window(x, y, x + w - 1, y + h - 1), TAG, "set fill window failed");
    for (int row = 0; row < h; ++row) {
        ESP_RETURN_ON_ERROR(lcd_data((const uint8_t *)line, w * 2), TAG, "fill row failed");
    }

    return ESP_OK;
}

static const uint8_t font_5x7[][5] = {
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00},
    ['!'] = {0x00, 0x00, 0x5f, 0x00, 0x00},
    ['-'] = {0x08, 0x08, 0x08, 0x08, 0x08},
    ['0'] = {0x3e, 0x51, 0x49, 0x45, 0x3e},
    ['1'] = {0x00, 0x42, 0x7f, 0x40, 0x00},
    ['2'] = {0x42, 0x61, 0x51, 0x49, 0x46},
    ['3'] = {0x21, 0x41, 0x45, 0x4b, 0x31},
    ['4'] = {0x18, 0x14, 0x12, 0x7f, 0x10},
    ['5'] = {0x27, 0x45, 0x45, 0x45, 0x39},
    ['6'] = {0x3c, 0x4a, 0x49, 0x49, 0x30},
    ['7'] = {0x01, 0x71, 0x09, 0x05, 0x03},
    ['8'] = {0x36, 0x49, 0x49, 0x49, 0x36},
    ['9'] = {0x06, 0x49, 0x49, 0x29, 0x1e},
    [':'] = {0x00, 0x36, 0x36, 0x00, 0x00},
    ['A'] = {0x7e, 0x11, 0x11, 0x11, 0x7e},
    ['B'] = {0x7f, 0x49, 0x49, 0x49, 0x36},
    ['C'] = {0x3e, 0x41, 0x41, 0x41, 0x22},
    ['D'] = {0x7f, 0x41, 0x41, 0x22, 0x1c},
    ['E'] = {0x7f, 0x49, 0x49, 0x49, 0x41},
    ['F'] = {0x7f, 0x09, 0x09, 0x09, 0x01},
    ['G'] = {0x3e, 0x41, 0x49, 0x49, 0x7a},
    ['H'] = {0x7f, 0x08, 0x08, 0x08, 0x7f},
    ['I'] = {0x00, 0x41, 0x7f, 0x41, 0x00},
    ['J'] = {0x20, 0x40, 0x41, 0x3f, 0x01},
    ['K'] = {0x7f, 0x08, 0x14, 0x22, 0x41},
    ['L'] = {0x7f, 0x40, 0x40, 0x40, 0x40},
    ['M'] = {0x7f, 0x02, 0x0c, 0x02, 0x7f},
    ['N'] = {0x7f, 0x04, 0x08, 0x10, 0x7f},
    ['O'] = {0x3e, 0x41, 0x41, 0x41, 0x3e},
    ['P'] = {0x7f, 0x09, 0x09, 0x09, 0x06},
    ['Q'] = {0x3e, 0x41, 0x51, 0x21, 0x5e},
    ['R'] = {0x7f, 0x09, 0x19, 0x29, 0x46},
    ['S'] = {0x46, 0x49, 0x49, 0x49, 0x31},
    ['T'] = {0x01, 0x01, 0x7f, 0x01, 0x01},
    ['U'] = {0x3f, 0x40, 0x40, 0x40, 0x3f},
    ['V'] = {0x1f, 0x20, 0x40, 0x20, 0x1f},
    ['W'] = {0x3f, 0x40, 0x38, 0x40, 0x3f},
    ['X'] = {0x63, 0x14, 0x08, 0x14, 0x63},
    ['Y'] = {0x07, 0x08, 0x70, 0x08, 0x07},
    ['Z'] = {0x61, 0x51, 0x49, 0x45, 0x43},
};

static esp_err_t draw_char(int x, int y, char ch, uint16_t fg, uint16_t bg, int scale)
{
    if ((unsigned char)ch >= sizeof(font_5x7) / sizeof(font_5x7[0])) {
        ch = ' ';
    }

    for (int col = 0; col < 6; ++col) {
        uint8_t bits = (col < 5) ? font_5x7[(int)ch][col] : 0x00;
        for (int row = 0; row < 8; ++row) {
            uint16_t color = (bits & (1 << row)) ? fg : bg;
            ESP_RETURN_ON_ERROR(lcd_fill_rect(x + col * scale, y + row * scale, scale, scale, color),
                                TAG, "draw char failed");
        }
    }

    return ESP_OK;
}

static esp_err_t draw_text(int x, int y, const char *text, uint16_t fg, uint16_t bg, int scale)
{
    while (*text) {
        ESP_RETURN_ON_ERROR(draw_char(x, y, *text, fg, bg, scale), TAG, "draw text failed");
        x += 6 * scale;
        ++text;
    }
    return ESP_OK;
}

static esp_err_t lcd_init(void)
{
    const uint8_t madctl = 0x28;
    const uint8_t pixfmt = 0x55;
    const uint8_t frctr1[] = {0x00, 0x18};
    const uint8_t dfunctr[] = {0x0a, 0x82, 0x27, 0x00};
    const uint8_t pwctr1 = 0x23;
    const uint8_t pwctr2 = 0x10;
    const uint8_t vmctr1[] = {0x3e, 0x28};
    const uint8_t vmctr2 = 0x86;
    const uint8_t gamma = 0x01;

    lcd_reset();
    ESP_RETURN_ON_ERROR(lcd_cmd(0x01), TAG, "software reset failed");
    vTaskDelay(pdMS_TO_TICKS(120));

    ESP_RETURN_ON_ERROR(lcd_cmd_data(0x3a, &pixfmt, 1), TAG, "pixel format failed");
    ESP_RETURN_ON_ERROR(lcd_cmd_data(0x36, &madctl, 1), TAG, "madctl failed");
    ESP_RETURN_ON_ERROR(lcd_cmd_data(0xb1, frctr1, sizeof(frctr1)), TAG, "frame control failed");
    ESP_RETURN_ON_ERROR(lcd_cmd_data(0xb6, dfunctr, sizeof(dfunctr)), TAG, "display function failed");
    ESP_RETURN_ON_ERROR(lcd_cmd_data(0xc0, &pwctr1, 1), TAG, "power 1 failed");
    ESP_RETURN_ON_ERROR(lcd_cmd_data(0xc1, &pwctr2, 1), TAG, "power 2 failed");
    ESP_RETURN_ON_ERROR(lcd_cmd_data(0xc5, vmctr1, sizeof(vmctr1)), TAG, "vcom 1 failed");
    ESP_RETURN_ON_ERROR(lcd_cmd_data(0xc7, &vmctr2, 1), TAG, "vcom 2 failed");
    ESP_RETURN_ON_ERROR(lcd_cmd_data(0x26, &gamma, 1), TAG, "gamma failed");
    ESP_RETURN_ON_ERROR(lcd_cmd(0x11), TAG, "sleep out failed");
    vTaskDelay(pdMS_TO_TICKS(120));
    ESP_RETURN_ON_ERROR(lcd_cmd(0x29), TAG, "display on failed");
    vTaskDelay(pdMS_TO_TICKS(20));

    return ESP_OK;
}

void app_main(void)
{
    ESP_LOGI(TAG, "display hello starting");
    ESP_LOGI(TAG, "pins: CLK=%d MOSI=%d CS=%d DC=%d RST=%d BL=%d",
             PIN_NUM_CLK, PIN_NUM_MOSI, PIN_NUM_CS, PIN_NUM_DC, PIN_NUM_RST, PIN_NUM_BKLT);

    gpio_config_t out_conf = {
        .pin_bit_mask = (1ULL << PIN_NUM_DC) | (1ULL << PIN_NUM_RST) | (1ULL << PIN_NUM_BKLT),
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&out_conf));
    gpio_set_level(PIN_NUM_BKLT, 1);

    spi_bus_config_t bus_conf = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * 40 * 2,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_conf, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t dev_conf = {
        .clock_speed_hz = 40 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(LCD_HOST, &dev_conf, &s_lcd));
    ESP_ERROR_CHECK(lcd_init());

    ESP_ERROR_CHECK(lcd_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_NAVY));
    ESP_ERROR_CHECK(lcd_fill_rect(0, 0, LCD_WIDTH, 34, COLOR_BLUE));
    ESP_ERROR_CHECK(lcd_fill_rect(0, 206, LCD_WIDTH, 34, COLOR_GREEN));
    ESP_ERROR_CHECK(lcd_fill_rect(16, 58, 288, 82, COLOR_BLACK));
    ESP_ERROR_CHECK(lcd_fill_rect(20, 62, 280, 74, COLOR_CYAN));
    ESP_ERROR_CHECK(draw_text(38, 78, "HELLO ESP32", COLOR_BLACK, COLOR_CYAN, 4));
    ESP_ERROR_CHECK(draw_text(38, 150, "ILI9341 UI TEST", COLOR_WHITE, COLOR_NAVY, 2));
    ESP_ERROR_CHECK(draw_text(38, 180, "COM20  320X240", COLOR_YELLOW, COLOR_NAVY, 2));

    int count = 0;
    while (true) {
        char buf[24];
        snprintf(buf, sizeof(buf), "COUNT %04d", count++);
        ESP_ERROR_CHECK(lcd_fill_rect(38, 210, 160, 22, COLOR_GREEN));
        ESP_ERROR_CHECK(draw_text(38, 210, buf, COLOR_BLACK, COLOR_GREEN, 2));
        ESP_LOGI(TAG, "%s", buf);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
