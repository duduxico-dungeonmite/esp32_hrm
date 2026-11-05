
// LEMBRAR DE FAZER git pull QUANDO EDITAR O PROJETO NA ESCOLA PRA SINCRONIZAR EM CASA!!!

#include <stdint.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_st7789.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_lvgl_port_disp.h"

#define LCD_HOST       SPI2_HOST

#define PIN_MOSI       GPIO_NUM_13
#define PIN_SCLK       GPIO_NUM_14
#define PIN_CS         GPIO_NUM_15
#define PIN_DC         GPIO_NUM_2
#define PIN_BACKLIGHT  GPIO_NUM_27

#define LCD_WIDTH      320
#define LCD_HEIGHT     240

#define LVGL_BUFFER_HEIGHT  40

// LCD handles shared with LVGL.
static esp_lcd_panel_io_handle_t lcd_io = nullptr;
static esp_lcd_panel_handle_t lcd_panel = nullptr;
static lv_display_t *lvgl_display = nullptr;

// --------------------------------------------------
// BACKLIGHT
// --------------------------------------------------

static void init_backlight()
{
    gpio_config_t config = {};
    config.pin_bit_mask = 1ULL << PIN_BACKLIGHT;
    config.mode = GPIO_MODE_OUTPUT;

    ESP_ERROR_CHECK(gpio_config(&config));
    ESP_ERROR_CHECK(gpio_set_level(PIN_BACKLIGHT, 1));
}

// --------------------------------------------------
// DISPLAY INITIALIZATION
// --------------------------------------------------

static void init_display()
{
    // Configure SPI bus.
    spi_bus_config_t bus_config = {};
    bus_config.sclk_io_num = PIN_SCLK;
    bus_config.mosi_io_num = PIN_MOSI;
    bus_config.miso_io_num = -1;
    bus_config.quadwp_io_num = -1;
    bus_config.quadhd_io_num = -1;
    bus_config.max_transfer_sz =
        LCD_WIDTH * LVGL_BUFFER_HEIGHT * sizeof(uint16_t);

    ESP_ERROR_CHECK(spi_bus_initialize(
        LCD_HOST, &bus_config, SPI_DMA_CH_AUTO
    ));

    // Configure LCD communication.
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
        &lcd_io
    ));

    // Create ST7789 panel.
    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.reset_gpio_num = GPIO_NUM_NC;
    panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    panel_config.bits_per_pixel = 16;

    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(
        lcd_io,
        &panel_config,
        &lcd_panel
    ));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcd_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcd_panel));

    // Common CYD display configuration.
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(lcd_panel, false));

    // Turn display on.
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(lcd_panel, true));
}



// --------------------------------------------------
// HRM USER INTERFACE
// --------------------------------------------------


static void create_hrm_ui()
{
    lv_obj_t *screen = lv_screen_active();

    // Catppuccin-inspired palette.
    const lv_color_t bg      = lv_color_hex(0x1C1C1C);
    const lv_color_t surface = lv_color_hex(0x1E1E2E);
    const lv_color_t purple  = lv_color_hex(0xB4A0E5);
    const lv_color_t green   = lv_color_hex(0x507963);
    const lv_color_t yellow  = lv_color_hex(0xBA8E23);
    const lv_color_t text    = lv_color_hex(0xCDD6F4);
    const lv_color_t muted   = lv_color_hex(0x6C7086);

    // Main background.
    lv_obj_set_style_bg_color(screen, bg, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(screen, 8, 0);
    lv_obj_set_style_border_width(screen, 0, 0);

    // ---------------- HEADER ----------------

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Core-X Mobile H.R.M");
    lv_obj_set_style_text_color(title, purple, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *status = lv_label_create(screen);
    lv_label_set_text(status, "! SENSOR DESCONECTADO !");
    lv_obj_set_style_text_color(status, yellow, 0);
    lv_obj_set_style_text_font(status, &lv_font_montserrat_12, 0);
    lv_obj_align(status, LV_ALIGN_TOP_LEFT, 0, 26);

    // ---------------- BPM CARD ----------------

    lv_obj_t *bpm_card = lv_obj_create(screen);
    lv_obj_set_size(bpm_card, 216, 76);
    lv_obj_align(bpm_card, LV_ALIGN_TOP_MID, 0, 48);

    lv_obj_set_style_bg_color(bpm_card, surface, 0);
    lv_obj_set_style_bg_opa(bpm_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bpm_card, 0, 0);
    lv_obj_set_style_radius(bpm_card, 12, 0);
    lv_obj_set_style_pad_all(bpm_card, 8, 0);

    lv_obj_t *bpm_caption = lv_label_create(bpm_card);
    lv_label_set_text(bpm_caption, "Freq. Cardiaca");
    lv_obj_set_style_text_color(bpm_caption, muted, 0);
    lv_obj_align(bpm_caption, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *bpm_value = lv_label_create(bpm_card);
    lv_label_set_text(bpm_value, "--");
    lv_obj_set_style_text_color(bpm_value, green, 0);
    lv_obj_set_style_text_font(bpm_value, &lv_font_montserrat_36, 0);
    lv_obj_align(bpm_value, LV_ALIGN_CENTER, -12, 8);

    lv_obj_t *bpm_unit = lv_label_create(bpm_card);
    lv_label_set_text(bpm_unit, "BPM");
    lv_obj_set_style_text_color(bpm_unit, text, 0);
    lv_obj_set_style_text_font(bpm_unit, &lv_font_montserrat_12, 0);
    lv_obj_align(bpm_unit, LV_ALIGN_RIGHT_MID, -8, 8);

    // ---------------- ECG CARD ----------------

    lv_obj_t *ecg_card = lv_obj_create(screen);
    lv_obj_set_size(ecg_card, 296, 72);
    lv_obj_align(ecg_card, LV_ALIGN_TOP_MID, 0, 132);

    lv_obj_set_style_bg_color(ecg_card, surface, 0);
    lv_obj_set_style_bg_opa(ecg_card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ecg_card, 0, 0);
    lv_obj_set_style_radius(ecg_card, 12, 0);
    lv_obj_set_style_pad_all(ecg_card, 8, 0);

    lv_obj_t *ecg_title = lv_label_create(ecg_card);
    lv_label_set_text(ecg_title, "SINAL ECG");
    lv_obj_set_style_text_color(ecg_title, purple, 0);
    lv_obj_align(ecg_title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *ecg_wait = lv_label_create(ecg_card);
    lv_label_set_text(ecg_wait, "Esperando sinal...");
    lv_obj_set_style_text_color(ecg_wait, muted, 0);
    lv_obj_set_style_text_font(ecg_wait, &lv_font_montserrat_12, 0);
    lv_obj_align(ecg_wait, LV_ALIGN_CENTER, 0, 8);

    // ---------------- FOOTER ----------------

    lv_obj_t *footer = lv_label_create(screen);
    lv_label_set_text(footer, "Sinal: --    Sessao/tempo: 00:00");
    lv_obj_set_style_text_color(footer, text, 0);
    lv_obj_set_style_text_font(footer, &lv_font_montserrat_12, 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_LEFT, 0, 0);
}


// --------------------------------------------------
// LVGL INITIALIZATION
// --------------------------------------------------

static void init_lvgl()
{
    // Initialize LVGL task and timer.
    const lvgl_port_cfg_t lvgl_config =
        ESP_LVGL_PORT_INIT_CONFIG();

    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_config));

    // Register the existing ST7789 display.
    lvgl_port_display_cfg_t display_config = {};

    display_config.io_handle = lcd_io;
    display_config.panel_handle = lcd_panel;

    display_config.buffer_size =
        LCD_WIDTH * LVGL_BUFFER_HEIGHT;

    display_config.double_buffer = true;

    display_config.hres = LCD_WIDTH;
    display_config.vres = LCD_HEIGHT;
    display_config.monochrome = false;

    display_config.color_format = LV_COLOR_FORMAT_RGB565;

    display_config.rotation.swap_xy = true;
    display_config.rotation.mirror_x = false;
    display_config.rotation.mirror_y = true;

    display_config.flags.buff_dma = true;
    display_config.flags.swap_bytes = true;

    lvgl_display = lvgl_port_add_disp(&display_config);

    // Create UI while holding LVGL's mutex.
    if (lvgl_port_lock(0)) {
        create_hrm_ui();
        lvgl_port_unlock();
    }
}


// --------------------------------------------------
// APPLICATION ENTRY POINT
// --------------------------------------------------


extern "C" void app_main()
{
    init_backlight();
    init_display();
    init_lvgl();

}

// LEMBRAR DE FAZER git pull QUANDO EDITAR O PROJETO NA ESCOLA PRA SINCRONIZAR EM CASA!!
