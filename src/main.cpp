#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"

// --- ピン定義 (Waveshare RP2350-Touch-LCD-2.8 専用) ---
#define LCD_SCLK 10
#define LCD_MOSI 11
#define LCD_CS   13
#define LCD_DC   14
#define LCD_RST  15
#define LCD_BL   16

// --- タッチパネル(CST328)ピン定義 ---
#define TP_SDA   6
#define TP_SCL   7
#define TP_RST   17
#define TP_INT   18
#define I2C_PORT i2c1 // ピン6,7は i2c1 に対応

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

// --- I2C バススキャン関数 ---
void i2c_scan() {
    printf("\nI2C Bus Scan\n");
    printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

    for (int addr = 0; addr < (1 << 7); ++addr) {
        if (addr % 16 == 0) {
            printf("%02x ", addr);
        }

        // 予約済みアドレスのスキップ
        int ret;
        uint8_t rxdata;
        if ((addr & 0x78) == 0 || (addr & 0x78) == 0x78) {
            ret = PICO_ERROR_GENERIC;
        } else {
            // アドレスに対して1バイトの読み出しを試みる
            ret = i2c_read_blocking(I2C_PORT, addr, &rxdata, 1, false);
        }

        printf(ret < 0 ? "." : "@");
        printf(addr % 16 == 15 ? "\n" : "  ");
    }
    printf("Done.\n");
}

// --- タッチパネル (CST328) アドレスとレジスタ ---
#define CST328_ADDR 0x1A

// タッチ座標を読み取る関数
bool read_touch_coordinates(uint16_t *x, uint16_t *y) {
    // 1. CST328の正しいデータ領域 (0xD000) を指定
    uint8_t reg[2] = {0xD0, 0x00};
    uint8_t buf[7];

    if (i2c_write_blocking(I2C_PORT, CST328_ADDR, reg, 2, true) == PICO_ERROR_GENERIC) return false;
    if (i2c_read_blocking(I2C_PORT, CST328_ADDR, buf, 7, false) == PICO_ERROR_GENERIC) return false;

    // 【追加】読み取った生データ（7バイト）をそのまま出力して解析する
    // ※画面に触れていない時も常に出力されるため、ログが高速で流れます
    //printf("Raw buf: %02X %02X %02X %02X %02X %02X %02X\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);

    // 2. チップに「データ受け取ったよ」と報告してバッファを消去（無限ループ防止）
    uint8_t clear_cmd[3] = {0xD0, 0x00, 0x00};
    i2c_write_blocking(I2C_PORT, CST328_ADDR, clear_cmd, 3, false);

    // 3. buf[1] が「タッチされている指の数」
    if (buf[1] == 0) {
        return false; // 指が離れていたらここでピタッと止まる
    }

    // 4. buf[2]〜buf[5] が「1本目の指」の座標データ
    // 最上位のイベントフラグ・IDを 0x0F でマスクして除去
    uint16_t raw_x = ((buf[2] & 0x0F) << 8) | buf[3];
    uint16_t raw_y = ((buf[4] & 0x0F) << 8) | buf[5];

    // 5. 生データ(0〜4095) を画面のピクセルサイズ (240 x 320) にスケール変換
    *x = (raw_x * 240) / 4095;
    *y = (raw_y * 320) / 4095;

    return true;
}

int main() {
    stdio_init_all();

    // 1. SPIの初期化 (省略: 変更なし)
    spi_init(SPI_PORT, 10 * 1000 * 1000);
    gpio_set_function(LCD_SCLK, GPIO_FUNC_SPI);
    gpio_set_function(LCD_MOSI, GPIO_FUNC_SPI);

    // 2. GPIOの初期化 (省略: 変更なし)
    gpio_init(LCD_RST); gpio_set_dir(LCD_RST, GPIO_OUT);
    gpio_init(LCD_DC);  gpio_set_dir(LCD_DC, GPIO_OUT);
    gpio_init(LCD_CS);  gpio_set_dir(LCD_CS, GPIO_OUT); gpio_put(LCD_CS, 1);
    gpio_init(LCD_BL);  gpio_set_dir(LCD_BL, GPIO_OUT); gpio_put(LCD_BL, 1);

    // 3. LCDの初期化とカラーテスト (省略: 変更なし)
    printf("ST7789 Initializing...\n");
    lcd_init();

    for ( int color_test_count = 0; color_test_count < 5; color_test_count++ ) {
        printf("\n--- Color Test Loop: %d / 5 ---\n", color_test_count + 1);
        printf("Test: RED\n");   lcd_fill_color(COLOR_RED);   sleep_ms(500);
        printf("Test: GREEN\n"); lcd_fill_color(COLOR_GREEN); sleep_ms(500);
        printf("Test: BLUE\n");  lcd_fill_color(COLOR_BLUE);  sleep_ms(500);
        printf("Test: WHITE\n"); lcd_fill_color(COLOR_WHITE); sleep_ms(500);
        printf("Test: BLACK\n"); lcd_fill_color(COLOR_BLACK); sleep_ms(500);
    }
    printf("LCD Initialized.\n");

    // --- 4. タッチパネル(I2C) の初期化 ---
    gpio_init(TP_RST);
    gpio_set_dir(TP_RST, GPIO_OUT);
    gpio_put(TP_RST, 0); sleep_ms(50);
    gpio_put(TP_RST, 1); sleep_ms(50);

    // TP_INTピンを入力を設定し、プルアップする
    gpio_init(TP_INT);
    gpio_set_dir(TP_INT, GPIO_IN);
    gpio_pull_up(TP_INT);

    // ...以下、I2Cの初期化継続    
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(TP_SDA, GPIO_FUNC_I2C);
    gpio_set_function(TP_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(TP_SDA);
    gpio_pull_up(TP_SCL);

    printf("Touch Panel Ready. Waiting for touch...\n");

    // --- 5. メインループ (タッチ検知) ---
    uint16_t touch_x, touch_y;
    
    while (true) {
        // タッチ座標の読み取りを試みる
        if (read_touch_coordinates(&touch_x, &touch_y)) {
            // タッチされていたら座標を表示
            printf("Touched! X: %d, Y: %d\n", touch_x, touch_y);
        }
        
        // 高速ループしすぎないように少し待機 (約60FPS相当)
        sleep_ms(16);
    }

    return 0;
}