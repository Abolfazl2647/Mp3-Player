# MP3 Player — Wiring & Hardware Specification

## Board

**ESP32-D0WD-V3** (standard 38-pin DevKit)

---

## Power Rails

| Rail | Supplies |
|------|----------|
| 5V (VIN) | ESP32 DevKit (recommended for stable operation) |
| 3.3V (ESP32 output) | OLED display, Micro SD WEMOS D1, rotary encoder, buttons, **PCM5102A** |
| GND | All components share a common ground |

**Note:** PCM5102A powered from ESP32's 3.3V output (not 5V) to minimize audio noise/ripple.

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
| VIN | 3.3V | ESP32 3.3V output (cleaner power = less audio noise) |
| GND | GND | Common ground with ESP32 |
| BCK | GPIO 26 | I2S bit clock |
| LCK | GPIO 25 | I2S word select (LRCK) |
| DIN | GPIO 27 | I2S data |
| SCK | GND | Tie to GND (no master clock needed) |
| FMT | GND | Tie to GND (I2S standard format) |
| FLT | GND | Filter select — normal filter mode |
| DEMP | GND | De-emphasis — disabled |
| XSMT | GPIO 2 | Soft mute control — controlled by ESP32 (HIGH = unmute) |

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

### Buttons (4-Pin Momentary Switches)

| Button | ESP32 Pin | Wiring |
|--------|-----------|--------|
| Next Track | GPIO 4 | Between GPIO 4 and GND (internal pull-up) |
| Prev Track | GPIO 14 | Between GPIO 14 and GND (internal pull-up) |

**4-Pin Button Configuration:**
- 4-pin buttons have two internally-connected pairs (diagonal pins)
- Use any **two opposite diagonal pins** (e.g., top-left + bottom-right)
- One leg → GPIO pin, other leg → GND
- **No external pull-up resistors needed** — ESP32 provides internal pull-ups
- **Active LOW:** Pin reads LOW when button is pressed

**To identify the correct pins:**
- Use a multimeter on continuity mode
- Press the button → listen for beep (indicates connected pair)
- Use those pins for wiring

---

## Pin Summary Table

| GPIO | Function | Component |
|------|----------|-----------|
| 2 | PCM XSMT | PCM5102A soft mute control |
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

- **All button and encoder pins use ESP32 internal pull-ups** — no external resistors needed.
- **PCM5102A Control Pins:**
  - SCK, FMT, FLT, DEMP must be tied to GND (standard I2S configuration)
  - XSMT controlled by GPIO 2 (HIGH = unmute, LOW = mute)
- **Power Configuration:**
  - ESP32 VIN: 5V (or USB) for stable operation
  - PCM5102A: **3.3V from ESP32 output** (cleaner power = less audio distortion/noise)
  - All other modules: 3.3V from ESP32
  - **Common GND:** All components share the same ground rail
  - **Optional:** Add 100µF capacitor across PCM5102A VIN–GND for additional noise filtering

---

## Audio Quality Tips

**To minimize distortion and noise:**
1. **Lower software volume** if sound is harsh or clipping (set to 8–12 instead of max)
2. **Use shielded audio cables** from PCM5102A output to headphone jack
3. **Add 100µF bypass capacitor** across PCM5102A power pins (+ on VIN, − on GND)
4. **Keep I2S wires short and close together** to minimize interference
5. **Ensure solid solder joints** on all I2S pins (BCK, LCK, DIN)
6. **Check all control pins grounded** (SCK, FMT, FLT, DEMP → GND)

---

## SD Card Format

- **Must be FAT32** with `.mp3` files in root directory (`/`)
- Max 200 tracks supported
- MP3 bitrate: 128 kbps or higher recommended
