# RP2350-Touch-LCD-2.8 動作確認テストツール開発仕様書

## 1. プロジェクト概要
本ドキュメントは、Waveshare製「RP2350-Touch-LCD-2.8」に搭載された2.8インチLCDおよび静電容量方式タッチパネルの動作を検証するための、C/C++ベースのテストツール仕様を定義する。

## 2. 開発環境
* **ホストOS:** macOS
* **エディタ:** Visual Studio Code / Cursor (Antigravity)
* **拡張機能:** Raspberry Pi Pico (VS Code Extension)
* **SDK:** Raspberry Pi Pico SDK (Target: RP2350 / ARM GCC)
* **言語:** C++20

## 3. ハードウェア仕様・ピンアサイン
回路図（RP2350-Touch-LCD-2.8-Schematic.pdf）に基づき、以下のピン定義を使用して通信を行う。

### 3.1 LCD制御 (SPI接続)
| 機能 | RP2350 ピン (GPIO) | 備考 |
| :--- | :--- | :--- |
| LCD_SCLK | GP10 | SPI1 SCK |
| LCD_MOSI | GP11 | SPI1 TX |
| LCD_CS | GP9 | Active Low |
| LCD_DC | GP8 | Data/Command Select |
| LCD_RST | GP12 | Hardware Reset |
| LCD_BL | GP25 | Backlight (PWM制御推奨) |

### 3.2 タッチパネル制御 (I2C接続)
| 機能 | RP2350 ピン (GPIO) | 備考 |
| :--- | :--- | :--- |
| TP_SDA | GP6 | I2C1 SDA |
| TP_SCL | GP7 | I2C1 SCL |
| TP_INT | GP5 | Interrupt Input |
| TP_RST | GP13 | Reset Output |
| I2C Address | 0x15 または 0x5D | CST328 制御用 |

## 4. ソフトウェア機能要件

### 4.1 初期化シーケンス
1.  **システム:** `stdio_init_all()` によりシリアルデバッグを有効化。
2.  **LCD:** SPI通信速度を最大60MHzに設定。ST7789コマンドを送信し、320x240ドット、RGB565カラー、回転角(Portrait/Landscape)を設定。
3.  **TP:** I2C通信(400kHz)を初期化。`TP_RST`をLow/High制御してCST328をリセット。

### 4.2 テスト実施項目
1.  **Color Fill Test:**
    * 画面全体を 赤 -> 緑 -> 青 -> 白 -> 黒 の順で1秒ごとに塗りつぶす。
    * 目的：ドット抜け、バックライトのムラ、色再現性の確認。
2.  **Touch Trace Test (簡易ペイント):**
    * タッチ座標をリアルタイムで取得。
    * タッチされた座標に半径3pxの円を描画。
    * 画面端に「CLR」ボタン（矩形領域）を表示し、タッチで画面をクリア。
    * 目的：タッチ座標のキャリブレーション精度、応答速度の確認。
3.  **Serial Debug Output:**
    * タッチポイント数、X座標、Y座標をUSBシリアル経由で出力。

## 5. フォルダ構造案
```text
RP2350_LCD_Test/
├── CMakeLists.txt          # プロジェクト構成
├── pico_sdk_import.cmake   # SDKインポート
├── src/
│   ├── main.cpp            # メインロジック
│   ├── lcd_driver.hpp/.cpp # ST7789制御
│   ├── touch_driver.hpp/.cpp # CST328制御
│   └── font.h              # テキスト表示用フォント
└── build/                  # ビルド生成物

## 6. ビルド・書き込み手順
1.  VS CodeのRPi Pico拡張にて「Pico2」を選択し、CMakeの再構成を行う。
2.  `pico_set_board(pico2)` が適用されていることを確認。
3.  「Build」を実行し、`RP2350_LCD_Test.uf2` を生成。
4.  本体のBOOTSELボタンを押しながら接続し、生成されたUF2ファイルをコピー。

## 7. 判定基準
* **LCD:** 色の反転がなく、指定した色が全画面に表示されること。
* **Touch:** 指の動きに対して遅延なく描画が追従し、座標の逆転（ミラーリング）が発生していないこと。
