#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#define LCD_HOST SPI3_HOST

#define LCD_WIDTH  320
#define LCD_HEIGHT 240

#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   14
#define PIN_NUM_DC   27
#define PIN_NUM_RST  33
#define PIN_NUM_BKLT 32

#define PIN_TOUCH_SDA 21
#define PIN_TOUCH_SCL 22
#define TOUCH_ADDR    0x38

#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xffff
#define COLOR_RED     0xf800
#define COLOR_GREEN   0x07e0
#define COLOR_BLUE    0x001f
#define COLOR_CYAN    0x07ff
#define COLOR_YELLOW  0xffe0
#define COLOR_NAVY    0x0011
#define COLOR_DARK    0x18c3
#define COLOR_PANEL   0x4208
#define COLOR_ORANGE  0xfd20
#define COLOR_PURPLE  0x780f
#define COLOR_GRAY    0x8410

static const char *TAG = "touch_ui_shell";
static spi_device_handle_t s_lcd;
static i2c_master_bus_handle_t s_i2c_bus;
static i2c_master_dev_handle_t s_touch_dev;
static bool s_wifi_ready;

typedef struct {
    bool pressed;
    uint8_t points;
    uint16_t x;
    uint16_t y;
} touch_state_t;

typedef enum {
    SCREEN_HOME,
    SCREEN_IMAGE,
    SCREEN_TIMER,
    SCREEN_SLEEP,
    SCREEN_SETTINGS,
    SCREEN_WIFI,
    SCREEN_IMU,
} app_screen_t;

static esp_err_t draw_header(const char *right);

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

    if (x >= LCD_WIDTH || y >= LCD_HEIGHT || w == 0 || h == 0) {
        return ESP_OK;
    }
    if (x + w > LCD_WIDTH) {
        w = LCD_WIDTH - x;
    }
    if (y + h > LCD_HEIGHT) {
        h = LCD_HEIGHT - y;
    }

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
    ['.'] = {0x00, 0x60, 0x60, 0x00, 0x00},
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
        ESP_RETURN_ON_ERROR(draw_char(x, y, (char)toupper((unsigned char)*text), fg, bg, scale), TAG, "draw text failed");
        x += 6 * scale;
        ++text;
    }
    return ESP_OK;
}

static void sanitize_text(char *dst, size_t dst_size, const char *src)
{
    size_t out = 0;

    if (dst_size == 0) {
        return;
    }

    while (*src && out + 1 < dst_size) {
        unsigned char c = (unsigned char)*src++;
        if (isalnum(c) || c == ' ' || c == '-' || c == '.') {
            dst[out++] = (char)toupper(c);
        } else {
            dst[out++] = ' ';
        }
    }
    dst[out] = '\0';
}

static esp_err_t lcd_init(void)
{
    const uint8_t madctl = 0x08;
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

static esp_err_t touch_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t data[] = {reg, value};
    return i2c_master_transmit(s_touch_dev, data, sizeof(data), pdMS_TO_TICKS(100));
}

static esp_err_t touch_read_reg(uint8_t reg, uint8_t *value)
{
    return i2c_master_transmit_receive(s_touch_dev, &reg, 1, value, 1, pdMS_TO_TICKS(100));
}

static esp_err_t touch_read_bytes(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(s_touch_dev, &reg, 1, data, len, pdMS_TO_TICKS(100));
}

static esp_err_t touch_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .sda_io_num = PIN_TOUCH_SDA,
        .scl_io_num = PIN_TOUCH_SCL,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_config, &s_i2c_bus), TAG, "i2c bus init failed");

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = TOUCH_ADDR,
        .scl_speed_hz = 400000,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(s_i2c_bus, &dev_config, &s_touch_dev), TAG, "touch add failed");

    uint8_t dummy = 0;
    ESP_RETURN_ON_ERROR(touch_read_reg(0x00, &dummy), TAG, "touch controller not responding");
    ESP_RETURN_ON_ERROR(touch_write_reg(0x00, 0x00), TAG, "touch mode write failed");
    ESP_RETURN_ON_ERROR(touch_write_reg(0xa4, 0x00), TAG, "touch reference init failed");

    ESP_LOGI(TAG, "touch controller responded at 0x%02x", TOUCH_ADDR);
    return ESP_OK;
}

static esp_err_t touch_read(touch_state_t *state)
{
    uint8_t points = 0;
    uint8_t data[6] = {0};

    memset(state, 0, sizeof(*state));
    ESP_RETURN_ON_ERROR(touch_read_reg(0x02, &points), TAG, "touch points read failed");
    state->points = points & 0x0f;
    state->pressed = state->points > 0;

    if (!state->pressed) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(touch_read_bytes(0x03, data, sizeof(data)), TAG, "touch xy read failed");
    state->x = ((uint16_t)(data[0] & 0x0f) << 8) | data[1];
    state->y = ((uint16_t)(data[2] & 0x0f) << 8) | data[3];
    return ESP_OK;
}

static esp_err_t draw_tile(int x, int y, int w, int h, uint16_t color, const char *title, const char *sub)
{
    int title_scale = (w < 70) ? 2 : 3;
    int title_x = x + ((w < 70) ? 8 : 12);
    int title_y = y + ((w < 70) ? 12 : 16);
    int sub_x = x + ((w < 70) ? 8 : 12);
    int sub_y = y + ((w < 70) ? 36 : 48);

    ESP_RETURN_ON_ERROR(lcd_fill_rect(x, y, w, h, COLOR_WHITE), TAG, "tile border failed");
    ESP_RETURN_ON_ERROR(lcd_fill_rect(x + 2, y + 2, w - 4, h - 4, color), TAG, "tile fill failed");
    ESP_RETURN_ON_ERROR(draw_text(title_x, title_y, title, COLOR_WHITE, color, title_scale), TAG, "tile title failed");
    ESP_RETURN_ON_ERROR(draw_text(sub_x, sub_y, sub, COLOR_WHITE, color, 1), TAG, "tile sub failed");
    return ESP_OK;
}

static esp_err_t draw_status_row(int y, const char *name, bool ok)
{
    uint16_t state_color = ok ? COLOR_GREEN : COLOR_RED;

    ESP_RETURN_ON_ERROR(draw_text(34, y, name, COLOR_WHITE, COLOR_DARK, 2), TAG, "status name failed");
    ESP_RETURN_ON_ERROR(lcd_fill_rect(230, y - 2, 58, 22, state_color), TAG, "status pill failed");
    ESP_RETURN_ON_ERROR(draw_text(244, y + 2, ok ? "OK" : "FAIL", COLOR_BLACK, state_color, 1), TAG, "status state failed");
    return ESP_OK;
}

static esp_err_t draw_boot_check_screen(void)
{
    ESP_RETURN_ON_ERROR(lcd_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_DARK), TAG, "boot clear failed");
    ESP_RETURN_ON_ERROR(draw_header("SYS INIT"), TAG, "boot header failed");
    ESP_RETURN_ON_ERROR(draw_text(28, 52, "FACTORY CHECK", COLOR_YELLOW, COLOR_DARK, 3), TAG, "boot title failed");
    ESP_RETURN_ON_ERROR(draw_status_row(104, "LCD", true), TAG, "lcd status failed");
    ESP_RETURN_ON_ERROR(draw_status_row(132, "TOUCH", false), TAG, "touch pending failed");
    ESP_RETURN_ON_ERROR(draw_status_row(160, "PSRAM", false), TAG, "psram pending failed");
    ESP_RETURN_ON_ERROR(draw_status_row(188, "WIFI", false), TAG, "wifi pending failed");
    return ESP_OK;
}

static bool check_psram_probe(void)
{
    uint8_t *test = heap_caps_malloc(4096, MALLOC_CAP_SPIRAM);
    if (test == NULL) {
        return false;
    }

    for (int i = 0; i < 4096; ++i) {
        test[i] = (uint8_t)(i ^ 0xa5);
    }
    for (int i = 0; i < 4096; ++i) {
        if (test[i] != (uint8_t)(i ^ 0xa5)) {
            heap_caps_free(test);
            return false;
        }
    }

    heap_caps_free(test);
    return true;
}

static esp_err_t draw_header(const char *right)
{
    ESP_RETURN_ON_ERROR(lcd_fill_rect(0, 0, LCD_WIDTH, 36, COLOR_NAVY), TAG, "top bar failed");
    ESP_RETURN_ON_ERROR(draw_text(12, 10, "RUBIK CUBE-1", COLOR_WHITE, COLOR_NAVY, 2), TAG, "title failed");
    ESP_RETURN_ON_ERROR(draw_text(228, 12, right, COLOR_CYAN, COLOR_NAVY, 1), TAG, "tag failed");
    return ESP_OK;
}

static esp_err_t draw_ui_shell(void)
{
    ESP_RETURN_ON_ERROR(lcd_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_DARK), TAG, "clear failed");
    ESP_RETURN_ON_ERROR(draw_header("MAIN MENU"), TAG, "header failed");

    ESP_RETURN_ON_ERROR(draw_tile(72, 54, 176, 70, COLOR_PANEL, "IMU", "MOTION DEMO"), TAG, "imu tile failed");
    ESP_RETURN_ON_ERROR(draw_tile(20, 148, 44, 56, COLOR_BLUE, "SD", "IMG"), TAG, "sd tile failed");
    ESP_RETURN_ON_ERROR(draw_tile(79, 148, 44, 56, COLOR_ORANGE, "WI", "SCAN"), TAG, "wifi tile failed");
    ESP_RETURN_ON_ERROR(draw_tile(138, 148, 44, 56, COLOR_PURPLE, "TM", "RUN"), TAG, "timer tile failed");
    ESP_RETURN_ON_ERROR(draw_tile(197, 148, 44, 56, COLOR_GRAY, "SL", "OFF"), TAG, "sleep tile failed");
    ESP_RETURN_ON_ERROR(draw_tile(256, 148, 44, 56, COLOR_GREEN, "ST", "SET"), TAG, "setting tile failed");

    ESP_RETURN_ON_ERROR(draw_text(18, 216, "M5 CORE2 FACTORY FLOW REFERENCE", COLOR_CYAN, COLOR_DARK, 1), TAG, "hint failed");
    return ESP_OK;
}

static esp_err_t draw_placeholder_screen(const char *title, const char *line1, const char *line2)
{
    ESP_RETURN_ON_ERROR(lcd_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_DARK), TAG, "clear screen failed");
    ESP_RETURN_ON_ERROR(draw_header("TAP TOP HOME"), TAG, "header failed");
    ESP_RETURN_ON_ERROR(draw_text(24, 58, title, COLOR_YELLOW, COLOR_DARK, 3), TAG, "placeholder title failed");
    ESP_RETURN_ON_ERROR(lcd_fill_rect(18, 112, 284, 94, COLOR_BLACK), TAG, "placeholder panel failed");
    ESP_RETURN_ON_ERROR(draw_text(32, 130, line1, COLOR_WHITE, COLOR_BLACK, 2), TAG, "placeholder line1 failed");
    ESP_RETURN_ON_ERROR(draw_text(32, 164, line2, COLOR_CYAN, COLOR_BLACK, 2), TAG, "placeholder line2 failed");
    return ESP_OK;
}

static bool hit_rect(const touch_state_t *touch, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    return touch->x >= x && touch->x < x + w && touch->y >= y && touch->y < y + h;
}

static esp_err_t wifi_prepare(void)
{
    if (s_wifi_ready) {
        return ESP_OK;
    }

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "nvs erase failed");
        err = nvs_flash_init();
    }
    ESP_RETURN_ON_ERROR(err, TAG, "nvs init failed");
    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "netif init failed");

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_RETURN_ON_ERROR(err, TAG, "event loop failed");
    }

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "wifi init failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "wifi mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "wifi start failed");

    s_wifi_ready = true;
    return ESP_OK;
}

static esp_err_t draw_wifi_scan_screen(void)
{
    uint16_t ap_count = 0;
    wifi_ap_record_t records[5] = {0};
    uint16_t record_count = sizeof(records) / sizeof(records[0]);

    ESP_RETURN_ON_ERROR(lcd_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, COLOR_DARK), TAG, "clear wifi failed");
    ESP_RETURN_ON_ERROR(draw_header("WIFI-SCAN"), TAG, "header failed");
    ESP_RETURN_ON_ERROR(draw_text(24, 58, "SCANNING...", COLOR_YELLOW, COLOR_DARK, 2), TAG, "scan label failed");

    esp_err_t err = wifi_prepare();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wifi prepare failed: %s", esp_err_to_name(err));
        ESP_RETURN_ON_ERROR(lcd_fill_rect(18, 100, 284, 86, COLOR_BLACK), TAG, "wifi error panel failed");
        ESP_RETURN_ON_ERROR(draw_text(32, 124, "WIFI INIT FAILED", COLOR_RED, COLOR_BLACK, 2), TAG, "wifi error text failed");
        return ESP_OK;
    }

    wifi_scan_config_t scan_config = {
        .show_hidden = true,
    };
    err = esp_wifi_scan_start(&scan_config, true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wifi scan failed: %s", esp_err_to_name(err));
        ESP_RETURN_ON_ERROR(lcd_fill_rect(18, 100, 284, 86, COLOR_BLACK), TAG, "scan error panel failed");
        ESP_RETURN_ON_ERROR(draw_text(32, 124, "SCAN FAILED", COLOR_RED, COLOR_BLACK, 2), TAG, "scan error text failed");
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(esp_wifi_scan_get_ap_num(&ap_count), TAG, "ap count failed");
    ESP_RETURN_ON_ERROR(esp_wifi_scan_get_ap_records(&record_count, records), TAG, "ap records failed");

    ESP_RETURN_ON_ERROR(lcd_fill_rect(0, 48, LCD_WIDTH, 192, COLOR_DARK), TAG, "clear scan result failed");
    char line[40];
    snprintf(line, sizeof(line), "FOUND %u AP", ap_count);
    ESP_RETURN_ON_ERROR(draw_text(24, 54, line, COLOR_GREEN, COLOR_DARK, 2), TAG, "found failed");

    if (record_count == 0) {
        ESP_RETURN_ON_ERROR(draw_text(24, 96, "NO NETWORKS", COLOR_WHITE, COLOR_DARK, 2), TAG, "no networks failed");
        return ESP_OK;
    }

    for (int i = 0; i < record_count; ++i) {
        char ssid[18];
        sanitize_text(ssid, sizeof(ssid), (const char *)records[i].ssid);
        snprintf(line, sizeof(line), "%d %s", i + 1, ssid[0] ? ssid : "HIDDEN");
        ESP_RETURN_ON_ERROR(draw_text(24, 90 + i * 28, line, COLOR_WHITE, COLOR_DARK, 1), TAG, "ssid failed");
        snprintf(line, sizeof(line), "RSSI %d", records[i].rssi);
        ESP_RETURN_ON_ERROR(draw_text(210, 90 + i * 28, line, COLOR_CYAN, COLOR_DARK, 1), TAG, "rssi failed");
    }

    return ESP_OK;
}

static esp_err_t handle_tap(const touch_state_t *touch, app_screen_t *screen)
{
    if (*screen != SCREEN_HOME && touch->y < 40) {
        *screen = SCREEN_HOME;
        return draw_ui_shell();
    }

    if (*screen != SCREEN_HOME) {
        return ESP_OK;
    }

    if (hit_rect(touch, 72, 54, 176, 70)) {
        *screen = SCREEN_IMU;
        return draw_placeholder_screen("IMU", "MPU6886 DEMO", "NEXT STEP");
    }
    if (hit_rect(touch, 20, 148, 44, 56)) {
        *screen = SCREEN_IMAGE;
        return draw_placeholder_screen("SD", "SD IMAGE APP", "CARD TEST LATER");
    }
    if (hit_rect(touch, 79, 148, 44, 56)) {
        *screen = SCREEN_WIFI;
        return draw_wifi_scan_screen();
    }
    if (hit_rect(touch, 138, 148, 44, 56)) {
        *screen = SCREEN_TIMER;
        return draw_placeholder_screen("TIMER", "COUNTDOWN APP", "NEXT STEP");
    }
    if (hit_rect(touch, 197, 148, 44, 56)) {
        *screen = SCREEN_SLEEP;
        return draw_placeholder_screen("SLEEP", "LOW POWER APP", "NOT ENABLED");
    }
    if (hit_rect(touch, 256, 148, 44, 56)) {
        *screen = SCREEN_SETTINGS;
        return draw_placeholder_screen("SET", "TIME THEME", "VOLUME WIFI");
    }

    return ESP_OK;
}

void app_main(void)
{
    ESP_LOGI(TAG, "touch ui shell starting");
    ESP_LOGI(TAG, "lcd pins: CLK=%d MOSI=%d CS=%d DC=%d RST=%d BL=%d",
             PIN_NUM_CLK, PIN_NUM_MOSI, PIN_NUM_CS, PIN_NUM_DC, PIN_NUM_RST, PIN_NUM_BKLT);
    ESP_LOGI(TAG, "touch pins: SDA=%d SCL=%d ADDR=0x%02x", PIN_TOUCH_SDA, PIN_TOUCH_SCL, TOUCH_ADDR);

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
    ESP_ERROR_CHECK(draw_boot_check_screen());
    ESP_ERROR_CHECK(touch_init());
    ESP_ERROR_CHECK(draw_status_row(132, "TOUCH", true));
    bool psram_ok = check_psram_probe();
    ESP_ERROR_CHECK(draw_status_row(160, "PSRAM", psram_ok));
    ESP_ERROR_CHECK(draw_status_row(188, "WIFI", true));
    vTaskDelay(pdMS_TO_TICKS(900));
    ESP_ERROR_CHECK(draw_ui_shell());

    app_screen_t screen = SCREEN_HOME;
    bool was_pressed = false;
    while (true) {
        touch_state_t touch;
        esp_err_t err = touch_read(&touch);
        if (err == ESP_OK) {
            if (touch.pressed) {
                ESP_LOGI(TAG, "touch points=%u x=%u y=%u", touch.points, touch.x, touch.y);
            }
            if (touch.pressed && !was_pressed) {
                ESP_ERROR_CHECK(handle_tap(&touch, &screen));
            }
            was_pressed = touch.pressed;
        } else {
            ESP_LOGW(TAG, "touch read failed: %s", esp_err_to_name(err));
            if (screen == SCREEN_HOME) {
                ESP_ERROR_CHECK(lcd_fill_rect(104, 166, 190, 48, COLOR_BLACK));
                ESP_ERROR_CHECK(draw_text(104, 180, "READ ERR", COLOR_RED, COLOR_BLACK, 2));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
