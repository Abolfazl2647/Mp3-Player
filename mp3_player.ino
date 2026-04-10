// ============================================================
//  MP3 Player — Main Entry Point
//  Board:  ESP32-D0WD-V3
//  Author: Auto-generated
// ============================================================
//
//  REQUIRED LIBRARIES (install via Arduino Library Manager):
//    1. ESP8266Audio      — MP3 decoding + I2S output
//    2. Adafruit SSD1306  — OLED driver
//    3. Adafruit GFX      — Graphics primitives
//    4. SD (built-in)     — FAT32 SD card access
//
//  HARDWARE NOTES:
//    - No external pull-up resistors needed
//    - PCM5102A: tie SCK to GND, FMT to GND (I2S standard) - XSMT pin to 3V3
//    - Power ESP32 + PCM5102A via VIN (5V)
//    - Power OLED + SD + buttons via 3.3V
//
// ============================================================
// ================ WIRING SUMMARY ===============

// ========== Display (I2C) ==========
// | SDA → GPIO 21 | SCL → GPIO 22 | VCC → 3.3V | GND → GND |

// ========== SD Card (SPI) ==========
// | CS → GPIO 5 | MOSI → GPIO 23 | MISO → GPIO 19 | SCK → GPIO 18 | VCC → 3.3V | GND → GND |

// ========== PCM5102A (I2S) ==========
// | BCK → GPIO 26 | LCK → GPIO 25 | DIN → GPIO 27 | SCK → GND | FMT → GND | VIN → 5V | GND → GND |

// ========== Rotary Encoder ==========
// | CLK → GPIO 32 | DT → GPIO 33 | SW → GPIO 13 | GND → GND |

// ========== Buttons (each button wires between pin and GND)
// | Next → GPIO 4 | Prev → GPIO 14 |

#include "Config.h"
#include "AppController.h"

AppController app;

void setup()
{
  app.begin();
}

void loop()
{
  app.update();
}
