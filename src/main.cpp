#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

// --- ピン定義 (Waveshare RP2350-Touch-LCD-2.8 専用) ---
#define LCD_SCLK 10
#define LCD_MOSI 11
#define LCD_CS   13
#define LCD_DC   14
#define LCD_RST  15
#define LCD_BL   16

// 使用するSPIポート (GPIO10, 11は spi1 に対応)
#define SPI_PORT spi1

// --- 色定義 (16bit RGB565フォーマット) ---
#define COLOR_RED   0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_BLUE  0x001F
#define COLOR_WHITE 0xFFFF
#define COLOR_BLACK 0x0000

// --- LCD制御用ヘルパー関数 ---

// コマンド送信
void lcd_write_cmd(uint8_t cmd) {
    gpio_put(LCD_CS, 0);
    gpio_put(LCD_DC, 0); // 0 = コマンドモード
    spi_write_blocking(SPI_PORT, &cmd, 1);
    gpio_put(LCD_CS, 1);
}

// データ送信
void lcd_write_data(uint8_t data) {
    gpio_put(LCD_CS, 0);
    gpio_put(LCD_DC, 1); // 1 = データモード
    spi_write_blocking(SPI_PORT, &data, 1);
    gpio_put(LCD_CS, 1);
}

// ST7789 初期化シーケンス
void lcd_init() {
    gpio_put(LCD_RST, 1); sleep_ms(50);
    gpio_put(LCD_RST, 0); sleep_ms(50);
    gpio_put(LCD_RST, 1); sleep_ms(150);

    lcd_write_cmd(0x36); lcd_write_data(0x00); // MADCTL
    lcd_write_cmd(0x3A); lcd_write_data(0x05); // COLMOD (RGB565)
    lcd_write_cmd(0x21); // INVON (IPS反転)
    lcd_write_cmd(0x11); // SLPOUT
    sleep_ms(120);
    lcd_write_cmd(0x29); // DISPON
    sleep_ms(50);
}

// 画面を指定した色(RGB565)で塗りつぶす
void lcd_fill_color(uint16_t color) {
    // 描画範囲を 240x320 に設定
    lcd_write_cmd(0x2A); // CASET
    lcd_write_data(0x00); lcd_write_data(0x00);
    lcd_write_data(0x00); lcd_write_data(0xEF); // 239

    lcd_write_cmd(0x2B); // RASET
    lcd_write_data(0x00); lcd_write_data(0x00);
    lcd_write_data(0x01); lcd_write_data(0x3F); // 319

    lcd_write_cmd(0x2C); // RAMWR

    gpio_put(LCD_CS, 0);
    gpio_put(LCD_DC, 1); // データモード
    
    // 16bitの色データを上位8bit・下位8bitに分割
    uint8_t color_buf[2] = { (uint8_t)(color >> 8), (uint8_t)(color & 0xFF) };
    
    // 240 x 320 ピクセル分のデータを一気に転送
    for (int i = 0; i < 240 * 320; i++) {
        spi_write_blocking(SPI_PORT, color_buf, 2);
    }
    gpio_put(LCD_CS, 1);
}

int main() {
    stdio_init_all();
    
    // 1. SPIの初期化
    spi_init(SPI_PORT, 10 * 1000 * 1000);
    gpio_set_function(LCD_SCLK, GPIO_FUNC_SPI);
    gpio_set_function(LCD_MOSI, GPIO_FUNC_SPI);

    // 2. GPIOの初期化
    gpio_init(LCD_RST); gpio_set_dir(LCD_RST, GPIO_OUT);
    gpio_init(LCD_DC);  gpio_set_dir(LCD_DC, GPIO_OUT);
    gpio_init(LCD_CS);  gpio_set_dir(LCD_CS, GPIO_OUT); gpio_put(LCD_CS, 1);
    gpio_init(LCD_BL);  gpio_set_dir(LCD_BL, GPIO_OUT); gpio_put(LCD_BL, 1);

    // 3. LCDの初期化
    printf("ST7789 Initializing...\n");
    lcd_init();

    // 4. メインループ (カラーテスト実行)
    while (true) {
        printf("Test: RED\n");
        lcd_fill_color(COLOR_RED);
        sleep_ms(2000); // 2秒待機

        printf("Test: GREEN\n");
        lcd_fill_color(COLOR_GREEN);
        sleep_ms(2000);

        printf("Test: BLUE\n");
        lcd_fill_color(COLOR_BLUE);
        sleep_ms(2000);

        printf("Test: WHITE\n");
        lcd_fill_color(COLOR_WHITE);
        sleep_ms(2000);

        printf("Test: BLACK\n");
        lcd_fill_color(COLOR_BLACK);
        sleep_ms(2000);
    }

    return 0;
}