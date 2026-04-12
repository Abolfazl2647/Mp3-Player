#pragma once

#define DEBUG_SERIAL 0

// ============================================================
//  MP3 Player — Pin Mapping & Configuration
//  Board: ESP32-D0WD-V3
// ============================================================
//
//  PIN ASSIGNMENT TABLE
//  ┌─────────────┬──────┬────────────────────────────────┐
//  │ Function    │ Pin  │ Notes                          │
//  ├─────────────┼──────┼────────────────────────────────┤
//  │ I2C SDA     │ 21   │ OLED display                   │
//  │ I2C SCL     │ 22   │ OLED display                   │
//  │ SPI MOSI    │ 23   │ SD card (VSPI default)         │
//  │ SPI MISO    │ 19   │ SD card (VSPI default)         │
//  │ SPI SCK     │ 18   │ SD card (VSPI default)         │
//  │ SPI CS      │  5   │ SD card chip select            │
//  │ I2S BCLK    │ 26   │ PCM5102A bit clock             │
//  │ I2S LRCK    │ 25   │ PCM5102A word select           │
//  │ I2S DOUT    │ 27   │ PCM5102A data out              │
//  │ ROT CLK     │ 32   │ Rotary encoder A (interrupt)   │
//  │ ROT DT      │ 33   │ Rotary encoder B (interrupt)   │
//  │ ROT SW      │ 13   │ Rotary push (internal pull-up) │
//  │ BTN NEXT    │  4   │ Button 1 (internal pull-up)    │
//  │ BTN PREV    │ 14   │ Button 2 (internal pull-up)    │
//  └─────────────┴──────┴────────────────────────────────┘
//
//  No external pull-up resistors needed — all button/encoder
//  pins use internal pull-ups.
//

// ── I2C (OLED) ────────────────────────────────────────────
#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDR 0x3C

// ── SPI (SD card) ─────────────────────────────────────────
#define PIN_SD_CS 5
// MOSI=23, MISO=19, SCK=18 are VSPI defaults used by SD lib

// ── I2S (PCM5102A DAC) ───────────────────────────────────
#define PIN_I2S_BCLK 26
#define PIN_I2S_LRCK 25
#define PIN_I2S_DOUT 27

// ── Rotary encoder ────────────────────────────────────────
#define PIN_ROT_CLK 32
#define PIN_ROT_DT 33
#define PIN_ROT_SW 13 // internal pull-up

// ── Buttons ───────────────────────────────────────────────
#define PIN_BTN_NEXT 4 // internal pull-up
#define PIN_BTN_PREV 14

// ── Audio settings ────────────────────────────────────────
#define VOLUME_DEFAULT 15 // 0–21 (ESP8266Audio gain range)
#define VOLUME_MIN 0
#define VOLUME_MAX 21
#define VOLUME_STEP 1

// ── SD / file settings ───────────────────────────────────
#define MAX_TRACKS 200
#define MP3_ROOT_DIR "/"

// ── UI timing (ms) ───────────────────────────────────────
#define UI_REFRESH_MS 100  // ~10 FPS display refresh
#define SCROLL_STEP_MS 200 // marquee scroll speed
#define DEBOUNCE_MS 50     // button debounce

// ── FreeRTOS task settings ────────────────────────────────
#define AUDIO_TASK_STACK 4096
#define AUDIO_TASK_PRIO 2 // higher than default (1)
#define AUDIO_TASK_CORE 1 // run audio on core 1

// ── Rotary long press ────────────────────────────────────
#define LONG_PRESS_MS 1100 // ms held before long-press fires

// ── Bluetooth settings ────────────────────────────────────
#define BT_AUTO_CONNECT_TIMEOUT_MS 8000 // boot reconnect timeout
#define BT_SCAN_TIMEOUT_MS 25000        // scan timeout
#define BT_MAX_SAVED_DEVICES 3
#define BT_FRIENDLY_NAME "ESP32-MP3"
