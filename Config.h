#pragma once

#define DEBUG_SERIAL 1

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
//  │ SPI MOSI    │ 23   │ Micro SD WEMOS D1 (VSPI)       │
//  │ SPI MISO    │ 19   │ Micro SD WEMOS D1 (VSPI)       │
//  │ SPI SCK     │ 18   │ Micro SD WEMOS D1 (VSPI)       │
//  │ SPI CS      │  5   │ Micro SD WEMOS D1 chip select  │
//  │ I2S BCLK    │ 26   │ PCM5102A bit clock             │
//  │ I2S LRCK    │ 25   │ PCM5102A word select           │
//  │ I2S DOUT    │ 27   │ PCM5102A data out              │
//  │ PCM XSMT    │  2   │ PCM5102A soft mute (unmute)    │
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

// ── SPI (Micro SD WEMOS D1) ───────────────────────────────
#define PIN_SD_CS 5
// MOSI=23, MISO=19, SCK=18 are VSPI defaults used by SD lib
// Module powered from 3.3V — no level shifter needed

// ── I2S (PCM5102A DAC) ───────────────────────────────────
#define PIN_I2S_BCLK 26
#define PIN_I2S_LRCK 25
#define PIN_I2S_DOUT 27
#define PIN_PCM_XSMT 2  // PCM5102A soft mute control (unmute)

// ── 5-Way Navigation Module ──────────────────────────────
#define PIN_5WAY_U 33    // Up — scroll up in menu
#define PIN_5WAY_D 14    // Down — scroll down in menu
#define PIN_5WAY_L 32    // Left — previous track (NOW_PLAYING only)
#define PIN_5WAY_R 4     // Right — next track (NOW_PLAYING only)
#define PIN_5WAY_C 13    // Click — select / play-pause

// ── Volume Buttons ────────────────────────────────────────
#define PIN_VOL_UP 12    // Volume up (internal pull-up)
#define PIN_VOL_DOWN 15  // Volume down (internal pull-up)

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

