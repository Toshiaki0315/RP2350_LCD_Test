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
    // 1. ハードウェアリセット
    gpio_put(LCD_RST, 1);
    sleep_ms(50);
    gpio_put(LCD_RST, 0);
    sleep_ms(50);
    gpio_put(LCD_RST, 1);
    sleep_ms(150);

    // 2. ST7789 コマンド送信
    lcd_write_cmd(0x36); // MADCTL (メモリアクセス制御: 画面の向き等)
    lcd_write_data(0x00);

    lcd_write_cmd(0x3A); // COLMOD (カラーフォーマット)
    lcd_write_data(0x05); // 16bit/pixel (RGB565)

    lcd_write_cmd(0x21); // INVON (色反転ON: IPSパネル特有の必須コマンド)
    
    lcd_write_cmd(0x11); // SLPOUT (スリープ解除)
    sleep_ms(120);       // スリープ解除後は少し待つ必要がある

    lcd_write_cmd(0x29); // DISPON (ディスプレイ表示ON)
    sleep_ms(50);
}

// 画面を赤色(RGB565: 0xF800)で塗りつぶす
void lcd_fill_red() {
    // 描画範囲を 240x320 に設定
    lcd_write_cmd(0x2A); // CASET (列アドレス設定)
    lcd_write_data(0x00); lcd_write_data(0x00); // Start: 0
    lcd_write_data(0x00); lcd_write_data(0xEF); // End: 239

    lcd_write_cmd(0x2B); // RASET (行アドレス設定)
    lcd_write_data(0x00); lcd_write_data(0x00); // Start: 0
    lcd_write_data(0x01); lcd_write_data(0x3F); // End: 319

    lcd_write_cmd(0x2C); // RAMWR (メモリ書き込み開始)

    gpio_put(LCD_CS, 0);
    gpio_put(LCD_DC, 1); // データモード
    
    // 赤色データ (RGB565フォーマットで 0xF800)
    uint8_t color[2] = {0xF8, 0x00};
    
    // 240 x 320 ピクセル分のデータを転送
    for (int i = 0; i < 240 * 320; i++) {
        spi_write_blocking(SPI_PORT, color, 2);
    }
    gpio_put(LCD_CS, 1);
}

int main() {
    stdio_init_all();
    
    // 1. SPIの初期化 (ひとまず安全な10MHzで駆動)
    spi_init(SPI_PORT, 10 * 1000 * 1000);
    gpio_set_function(LCD_SCLK, GPIO_FUNC_SPI);
    gpio_set_function(LCD_MOSI, GPIO_FUNC_SPI);

    // 2. GPIOの初期化
    gpio_init(LCD_RST); gpio_set_dir(LCD_RST, GPIO_OUT);
    gpio_init(LCD_DC);  gpio_set_dir(LCD_DC, GPIO_OUT);
    gpio_init(LCD_CS);  gpio_set_dir(LCD_CS, GPIO_OUT); gpio_put(LCD_CS, 1);
    
    // バックライト点灯
    gpio_init(LCD_BL);  gpio_set_dir(LCD_BL, GPIO_OUT); gpio_put(LCD_BL, 1);

    // 3. LCDの初期化とテスト描画
    printf("ST7789 Initializing...\n");
    lcd_init();
    
    printf("Drawing Red Screen...\n");
    lcd_fill_red();

    // 4. メインループ
    while (true) {
        printf("RP2350-Touch-LCD-2.8: Screen should be RED!\n");
        sleep_ms(1000);
    }

    return 0;
}