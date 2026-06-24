# MP3 Player — Wiring & Hardware Specification

## Board

**ESP32-D0WD-V3** (standard 38-pin DevKit)

---

## Power Rails

| Rail | Supplies |
|------|----------|
| 5V (VIN) | ESP32 DevKit, PCM5102A |
| 3.3V | OLED display, Micro SD WEMOS D1, rotary encoder, buttons |
| GND | All components share a common ground |

---

## Component Wiring

### OLED Display — I2C (128×64, SSD1306)

| OLED Pin | ESP32 Pin | Notes |
|----------|-----------|-------|
| VCC | 3.3V | |
| GND | GND | |
| SDA | GPIO 21 | I2C data |
| SCL | GPIO 22 | I2C clock |

- I2C address: `0x3C`
- No external pull-up resistors needed (ESP32 internal pull-ups used)

---

### Micro SD Card Module — SPI (WEMOS D1 Mini SD Shield)

| SD Module Pin | ESP32 Pin | Notes |
|---------------|-----------|-------|
| VCC | 3.3V | Module is 3.3V — no level shifter needed |
| GND | GND | |
| CS | GPIO 5 | SPI chip select |
| MOSI | GPIO 23 | VSPI MOSI |
| MISO | GPIO 19 | VSPI MISO |
| SCK | GPIO 18 | VSPI clock |

- SPI bus: VSPI (hardware SPI)
- SD card format: FAT32

---

### PCM5102A DAC — I2S

| PCM5102A Pin | ESP32 Pin / Voltage | Notes |
|--------------|---------------------|-------|
| VIN | 5V | |
| GND | GND | |
| BCK | GPIO 26 | I2S bit clock |
| LCK | GPIO 25 | I2S word select (LRCK) |
| DIN | GPIO 27 | I2S data |
| SCK | GND | Tie to GND (no master clock needed) |
| FMT | GND | Tie to GND (I2S standard format) |
| XSMT | 3.3V | Soft mute control — tie HIGH to enable output |

---

### Rotary Encoder (with push button)

| Encoder Pin | ESP32 Pin | Notes |
|-------------|-----------|-------|
| VCC | 3.3V | (if module has VCC pin) |
| GND | GND | |
| CLK (A) | GPIO 32 | Interrupt-capable pin |
| DT (B) | GPIO 33 | Interrupt-capable pin |
| SW | GPIO 13 | Push button — internal pull-up enabled |

- GPIO 13 uses ESP32 internal pull-up; wire SW between GPIO 13 and GND
- Rotate: adjust volume / navigate menu
- Press: play/pause / confirm selection

---

### Buttons

| Button | ESP32 Pin | Wiring |
|--------|-----------|--------|
| Next Track | GPIO 4 | Between GPIO 4 and GND (internal pull-up) |
| Prev Track | GPIO 14 | Between GPIO 14 and GND (internal pull-up) |

- No external pull-up resistors needed
- Active LOW: pin reads LOW when button is pressed

---

## Pin Summary Table

| GPIO | Function | Component |
|------|----------|-----------|
| 4 | BTN NEXT | Next button |
| 5 | SPI CS | SD card chip select |
| 13 | ROT SW | Rotary encoder push button |
| 14 | BTN PREV | Prev button |
| 18 | SPI SCK | SD card clock |
| 19 | SPI MISO | SD card data in |
| 21 | I2C SDA | OLED display |
| 22 | I2C SCL | OLED display |
| 23 | SPI MOSI | SD card data out |
| 25 | I2S LRCK | PCM5102A word select |
| 26 | I2S BCLK | PCM5102A bit clock |
| 27 | I2S DOUT | PCM5102A data |
| 32 | ROT CLK | Rotary encoder A |
| 33 | ROT DT | Rotary encoder B |

---

## Software Configuration

| Parameter | Value |
|-----------|-------|
| Volume default | 15 (range: 0–21) |
| Volume step | 1 |
| Max tracks | 200 |
| SD root dir | `/` |
| UI refresh rate | 100 ms (~10 FPS) |
| Scroll step | 200 ms |
| Button debounce | 50 ms |
| Audio task stack | 4096 bytes |
| Audio task priority | 2 (above default) |
| Audio task core | Core 1 |

---

## Required Libraries

| Library | Purpose |
|---------|---------|
| ESP8266Audio | MP3 decoding + I2S output |
| Adafruit SSD1306 | OLED driver |
| Adafruit GFX | Graphics primitives |
| SD (built-in) | FAT32 SD card access |

---

## Notes

- All button and encoder pins use ESP32 internal pull-ups — no external resistors needed.
- PCM5102A SCK and FMT must be tied to GND; XSMT must be tied to 3.3V to unmute the DAC output.
- Power the ESP32 DevKit and PCM5102A from 5V; all other modules from the ESP32's 3.3V regulator.
- SD card must be formatted as FAT32.
