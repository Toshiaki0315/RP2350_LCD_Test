# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

# Raspberry Pi Pico SDK のインポート
include(pico_sdk_import.cmake)

project(RP2350_LCD_Test C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

# SDKの初期化
pico_sdk_init()

# 実行ファイルの追加
add_executable(RP2350_LCD_Test
    src/main.cpp
    # 今後ファイルが増えたらここに追加
    # src/lcd_driver.cpp
    # src/touch_driver.cpp
)

# リンクするライブラリの指定
# hardware_spi: LCD通信用
# hardware_i2c: タッチパネル通信用
# hardware_pwm: バックライト輝度制御用
target_link_libraries(RP2350_LCD_Test 
    pico_stdlib 
    hardware_spi 
    hardware_i2c 
    hardware_pwm
)

# ボードの指定（Pico 2 / RP2350）
set(PICO_BOARD pico2)

# 標準出力（printf等）をUSBシリアル経由に設定
pico_enable_stdio_usb(RP2350_LCD_Test 1)
pico_enable_stdio_uart(RP2350_LCD_Test 0)

# UF2ファイルの生成
pico_add_extra_outputs(RP2350_LCD_Test)
