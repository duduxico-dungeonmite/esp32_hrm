// LEMBRAR DE FAZER git pull QUANDO EDITAR O PROJETO NA ESCOLA PRA SINCRONIZAR EM CASA!!!

#include <stdint.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9341.h"

#define LCD_HOST       SPI2_HOST

#define PIN_MOSI       GPIO_NUM_13
#define PIN_SCLK       GPIO_NUM_14
#define PIN_CS         GPIO_NUM_15
#define PIN_DC         GPIO_NUM_2
#define PIN_BACKLIGHT  GPIO_NUM_21

#define LCD_WIDTH      240
#define LCD_HEIGHT     320

#define CHUNK_HEIGHT   16

static void init_backlight()
{
    gpio_config_t config = {};
    config.pin_bit_mask = 1ULL << PIN_BACKLIGHT;
    config.mode = GPIO_MODE_OUTPUT;

    ESP_ERROR_CHECK(gpio_config(&config));
    ESP_ERROR_CHECK(gpio_set_level(
        static_cast<gpio_num_t>(PIN_BACKLIGHT), 1
    ));
}

static esp_lcd_panel_handle_t init_display()
{
    // Configure SPI bus.
    spi_bus_config_t bus_config = {};
    bus_config.sclk_io_num = PIN_SCLK;
    bus_config.mosi_io_num = PIN_MOSI;
    bus_config.miso_io_num = -1;
    bus_config.quadwp_io_num = -1;
    bus_config.quadhd_io_num = -1;
    bus_config.max_transfer_sz =
        LCD_WIDTH * CHUNK_HEIGHT * sizeof(uint16_t);

    ESP_ERROR_CHECK(spi_bus_initialize(
        LCD_HOST, &bus_config, SPI_DMA_CH_AUTO
    ));

    // Configure LCD communication.
    esp_lcd_panel_io_handle_t io_handle = nullptr;

    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.cs_gpio_num = PIN_CS;
    io_config.dc_gpio_num = PIN_DC;
    io_config.spi_mode = 0;
    io_config.pclk_hz = 40 * 1000 * 1000;
    io_config.trans_queue_depth = 10;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        static_cast<esp_lcd_spi_bus_handle_t>(LCD_HOST),
        &io_config,
        &io_handle
    ));

    // Create ILI9341 panel.
    esp_lcd_panel_handle_t panel = nullptr;

    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.reset_gpio_num = GPIO_NUM_NC;
    panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    panel_config.bits_per_pixel = 16;

    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(
        io_handle,
        &panel_config,
        &panel
    ));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));

    // Common CYD display configuration.
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel, true));

    return panel;
}

static void draw_test_pattern(esp_lcd_panel_handle_t panel)
{
    static uint16_t buffer[LCD_WIDTH * CHUNK_HEIGHT];

    const uint16_t colors[] = {
        0xF800, // Red
        0x07E0, // Green
        0x001F, // Blue
        0xFFFF, // White
        0x0000  // Black
    };

    constexpr int band_height = LCD_HEIGHT / 5;

    for (int band = 0; band < 5; ++band) {
        const uint16_t color = colors[band];

        for (auto &pixel : buffer) {
            pixel = color;
        }

        for (int y = band * band_height;
             y < (band + 1) * band_height;
             y += CHUNK_HEIGHT) {

            ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(
                panel,
                0, y,
                LCD_WIDTH, y + CHUNK_HEIGHT,
                buffer
            ));
        }
    }
}

extern "C" void app_main()
{
    init_backlight();

    esp_lcd_panel_handle_t display = init_display();

    draw_test_pattern(display);
}

// LEMBRAR DE FAZER git pull QUANDO EDITAR O PROJETO NA ESCOLA PRA SINCRONIZAR EM CASA!!