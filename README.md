# ESP32 MP3 Player

A standalone MP3 player built on the ESP32-D0WD-V3, featuring I2S audio output via a PCM5102A DAC, a 128×64 OLED display, SD card playback, and rotary encoder + button controls. Audio decoding runs on a dedicated FreeRTOS task on Core 1, keeping the UI responsive on Core 0.

---

## Features

- MP3 playback from FAT32 micro SD card (up to 200 tracks)
- High-quality I2S audio via PCM5102A DAC
- 128×64 OLED display with scrolling track name, progress bar, and playback status
- Rotary encoder for volume control and menu navigation
- Next / Previous track buttons
- Play, pause, and resume support
- ~10 FPS display refresh with FreeRTOS task isolation

---

## Hardware Components

### ESP32 DevKit (ESP32-D0WD-V3)

<img src="image/esp32.jpg" alt="ESP32 DevKit" width="400"/>

The main microcontroller. Dual-core 240 MHz, Wi-Fi + BT (BT unused here), 38 pins.
Runs audio decoding on Core 1 and UI/input handling on Core 0.

---

### 0.96" OLED Display (SSD1306, 128×64, I2C)

<img src="image/oled.jpg" alt="SSD1306 OLED Display" width="400"/>

Monochrome 128×64 pixel display driven over I2C.
Shows track name (with marquee scroll), progress bar, elapsed/total time, volume level, and playback state.

| Pin | ESP32 GPIO |
|-----|-----------|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

---

### Micro SD Card Module (SPI / WEMOS D1 Shield)

<img src="image/sd_card.png" alt="Micro SD Card Module" width="400"/>

FAT32 micro SD card reader over SPI (VSPI bus). Stores all `.mp3` files in the root directory.
Powered from 3.3V — no level shifter needed.

| Pin  | ESP32 GPIO |
|------|-----------|
| VCC  | 3.3V |
| GND  | GND |
| CS   | GPIO 5 |
| MOSI | GPIO 23 |
| MISO | GPIO 19 |
| SCK  | GPIO 18 |

---

### PCM5102A I2S DAC

<img src="image/pcm5102a.webp" alt="PCM5102A DAC Module" width="400"/>

High-fidelity stereo DAC (112 dB SNR) driven over I2S. Outputs line-level audio to a 3.5mm jack.
SCK and FMT must be tied to GND; XSMT tied to 3.3V to unmute the output.

| Pin  | ESP32 GPIO / Rail |
|------|------------------|
| VIN  | 5V |
| GND  | GND |
| BCK  | GPIO 26 |
| LCK  | GPIO 25 |
| DIN  | GPIO 27 |
| SCK  | GND (tie) |
| FMT  | GND (tie) |
| XSMT | 3.3V (tie) |

---

### KY-040 Rotary Encoder

<img src="image/rotary_encoder.jpg" alt="KY-040 Rotary Encoder" width="400"/>

Rotary encoder with push button. Rotating adjusts volume / navigates menus; pressing plays/pauses or confirms selection.
All pins use ESP32 internal pull-ups — no external resistors needed.

| Pin | ESP32 GPIO |
|-----|-----------|
| GND | GND |
| CLK | GPIO 32 |
| DT  | GPIO 33 |
| SW  | GPIO 13 |

---

### Tactile Push Buttons

<img src="image/push_button.jpg" alt="Tactile Push Buttons" width="400"/>

Two momentary push buttons for Next Track and Previous Track.
Wired between the GPIO pin and GND; internal pull-ups used (active LOW).

| Button | ESP32 GPIO |
|--------|-----------|
| Next   | GPIO 4 |
| Prev   | GPIO 14 |

---

## Wiring

See [wiring.md](wiring.md) for the full pin reference table, power rail diagram, and all tie-off notes.

---

## Libraries

Install via Arduino Library Manager:

| Library | Purpose |
|---------|---------|
| [ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio) | MP3 decoding + I2S output |
| Adafruit SSD1306 | OLED driver |
| Adafruit GFX | Graphics primitives |
| SD *(built-in)* | FAT32 SD card access |

---

## Build & Flash

1. Open `mp3_player.ino` in Arduino IDE (2.x recommended).
2. Select board: **ESP32 Dev Module**.
3. Install the four libraries listed above.
4. Flash. Serial output is disabled by default (`DEBUG_SERIAL 0` in `Config.h`).

---

## Configuration

All tuneable constants live in [`Config.h`](Config.h):

| Constant | Default | Description |
|----------|---------|-------------|
| `VOLUME_DEFAULT` | 15 | Initial volume (0–21) |
| `VOLUME_MAX` | 21 | Max volume |
| `MAX_TRACKS` | 200 | Max files scanned on SD |
| `UI_REFRESH_MS` | 100 | Display refresh interval |
| `SCROLL_STEP_MS` | 200 | Marquee scroll speed |
| `DEBOUNCE_MS` | 50 | Button debounce window |
