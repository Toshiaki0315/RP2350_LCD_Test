# RP2350 Touch LCD Test

Waveshare製「RP2350-Touch-LCD-2.8」開発ボード向けのハードウェア初期化および動作確認（テスト）プログラムです。

Raspberry Pi Pico 2 (RP2350) をターゲットとし、搭載されているSPI液晶（ST7789T3）のカラー表示テストと、I2Cタッチパネル（CST328）のバススキャン（生存確認）を連続して実行します。

## 🚀 特徴 (Features)

1. **LCD表示テスト (ST7789T3)**
   - SPI通信（10MHz）によるディスプレイの初期化。
   - 画面全体を指定色（赤→緑→青→白→黒）で順番に塗りつぶし、ドット抜けやフリッカー（チラつき）などのハードウェア健康診断を行います。
2. **タッチパネルI2Cスキャン (CST328)**
   - I2C通信（400kHz）を立ち上げ、接続されているデバイスのアドレスをスキャンします。
   - シリアルモニタ上で `0x1A` 等のアドレスに応答（`@`）があることを確認できます。

## 🛠 ハードウェア環境

- **開発ボード**: Waveshare RP2350-Touch-LCD-2.8
- **MCU**: Raspberry Pi RP2350 (Pico 2 互換)
- **ディスプレイ**: 2.8インチ IPS LCD (ST7789T3コントローラ / 240x320ピクセル)
- **タッチパネル**: 静電容量方式 (CST328コントローラ)

## 📌 ピン配置 (Pinout)

この基板は、画面とマイコンが一体化しているため独自のピン配線となっています。

### 1. SPI 液晶ディスプレイ (ST7789T3)
| ピンの役割 | 信号名 (LCD側) | Pico 2 GPIOピン | 備考 |
| :--- | :--- | :--- | :--- |
| **MOSI** (データ送信) | LCD_MOSI | **GPIO 11** | SPI1 TX |
| **MISO** (データ受信) | LCD_MISO | **GPIO 12** | SPI1 RX (通常の描画では使用しない) |
| **SCK** (クロック) | LCD_SCK | **GPIO 10** | SPI1 SCK |
| **CS** (チップセレクト) | LCD_CS | **GPIO 13** | |
| **DC** (データ/コマンド切替)| LCD_D/C | **GPIO 14** | |
| **RST** (リセット) | LCD_RST | **GPIO 15** | |
| **BL** (バックライト) | LCD_BL | **GPIO 16** | PWM制御可能 |

### 2. タッチパネル (CST328 - I2C接続)
| ピンの役割 | 信号名 | Pico 2 GPIOピン | 備考 |
| :--- | :--- | :--- | :--- |
| **SDA** (データ) | TP_SDA | **GPIO 6** | I2C1 SDA |
| **SCL** (クロック) | TP_SCL | **GPIO 7** | I2C1 SCL |
| **INT** (割り込み) | TP_INT | **GPIO 18** | タッチ検知用 |
| **RST** (リセット) | TP_RST | **GPIO 17** | |

### 3. MicroSDカードスロット (参考情報)
| ピンの役割 (SPI) | 信号名 (SD側) | Pico 2 GPIOピン | 備考 |
| :--- | :--- | :--- | :--- |
| **SCK** (クロック) | SD_SCK | **GPIO 19** | SPI0 SCK |
| **MOSI** (データ送信) | SD_CMD | **GPIO 20** | SDIOのCMDピンをSPIのMOSIとして使用 |
| **MISO** (データ受信) | SD_D0 | **GPIO 21** | SDIOのD0ピンをSPIのMISOとして使用 |
| - (SPIでは未使用) | SD_D1 | **GPIO 22** | SDIO 4bit通信用データ線1 |
| - (SPIでは未使用) | SD_D2 | **GPIO 23** | SDIO 4bit通信用データ線2 |
| **CS** (チップセレクト) | SD_D3 | **GPIO 24** | SDIOのD3ピンをSPIのCSとして使用 |

## 💻 開発環境・ビルド方法

- **SDK**: Raspberry Pi Pico SDK 2.2.0 (以降)
- **ビルドツール**: CMake

### CMakeでのターゲット指定に関する注意
RP2350向けに正しくビルドするため、`CMakeLists.txt` 内でSDK初期化の前にボードとプラットフォームを明示的に指定しています。（VS Codeの拡張機能による自動上書きを防ぐための処置を含みます）

```cmake
# SDK初期化の前にPico 2 (RP2350) を強制指定
set(PICO_BOARD pico2 CACHE STRING "Board type" FORCE)
set(PICO_PLATFORM rp2350 CACHE STRING "Platform type" FORCE)
```

### ビルド手順 (CLI)
```bash
mkdir build
cd build
cmake ..
make -j8
```
生成された `RP2350_LCD_Test.uf2` を、BOOTSELモードで起動したPico 2へドラッグ＆ドロップしてください。

## 📺 実行結果 (Expected Behavior)

1. **画面のカラーテスト**:
   起動直後、画面が「赤 → 緑 → 青 → 白 → 黒」の順に0.5秒間隔で切り替わるループを5回実行します。
2. **I2Cスキャナー起動**:
   カラーテスト終了後、画面は黒のまま維持され、3秒おきにI2Cバスのスキャン結果をUSBシリアル経由でPCへ出力します。

**シリアルモニタ出力例:**
```text
ST7789 Initializing...

--- Color Test Loop: 1 / 5 ---
Test: RED
Test: GREEN
...
LCD Initialized.

I2C Bus Scan
   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
00 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
10 .  .  .  .  .  .  .  .  .  .  @  .  .  .  .  .  <-- 0x1A でデバイス(CST328)が応答
...
```
