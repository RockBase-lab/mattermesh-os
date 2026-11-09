#pragma once

#ifdef MM_BOARD_NM_EPD

// RockBase NM-EPD-420-BW (ESP32-S3 N16R8), from the NM-EPD-420 MeshCore variant.
#define BOARD_BOOT_PIN 0
#define BOARD_USER_BUTTON 45

// LoRa and microSD share HSPI. The e-paper panel has its own FSPI bus.
#define BOARD_SPI_SCK 9
#define BOARD_SPI_MISO 11
#define BOARD_SPI_MOSI 10
#define BOARD_SD_CS 7
#define BOARD_LORA_CS 8
#define BOARD_LORA_INT 14
#define BOARD_LORA_RST 12
#define BOARD_LORA_BUSY 13
#define BOARD_LORA_EN 47

#define BOARD_EPD_SCK 2
#define BOARD_EPD_MISO -1
#define BOARD_EPD_MOSI 1
#define BOARD_EPD_CS 46
#define BOARD_EPD_DC 4
#define BOARD_EPD_RST 5
#define BOARD_EPD_BUSY 6
#define BOARD_EPD_BL -1

#define BOARD_I2C_SDA 39
#define BOARD_I2C_SCL 38
#define BOARD_I2C_ADDR_ES8311 0x18
#define BOARD_ES8311_MCLK 21
#define BOARD_ES8311_SCLK 15
#define BOARD_ES8311_LRCK 17
#define BOARD_ES8311_ASDOUT 16
#define BOARD_ES8311_DSDIN 18
#define BOARD_ES8311_EN 44 // active-high codec power: LOW keeps it off
#define BOARD_AMPLIFIER_EN 41
#define BOARD_AHT20_EN 40

#define BOARD_BATTERY_ADC 3
#define BOARD_BATTERY_ADC_EN 43
#define BOARD_BATTERY_ADC_MULTIPLIER 2.0f

// Peripherals present on the T-Deck Max but not on this board.
#define BOARD_KEYBOARD_INT -1
#define BOARD_KEYBOARD_LED -1
#define BOARD_GPS_RXD -1
#define BOARD_GPS_TXD -1
#define SerialGPS Serial2 // compile-only: locationBegin() permanently disables GPS on this board
#define BOARD_XL9555_01_LORA_EN -1
#define BOARD_XL9555_02_GPS_EN -1
#define BOARD_XL9555_04_LORA_SEL -1
#define BOARD_XL9555_05_MOTOR_EN -1
#define BOARD_XL9555_06_AMPLIFIER -1
#define BOARD_XL9555_07_TOUCH_RST -1
#define BOARD_XL9555_11_KEY_RST -1
#define BOARD_XL9555_12_AUDIO_SEL -1

#else
#include <TDeckMaxBoard.h>
#endif
