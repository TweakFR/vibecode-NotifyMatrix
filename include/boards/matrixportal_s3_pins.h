#pragma once

#if !defined(BOARD_MATRIXPORTAL_S3)
#error "This file only targets BOARD_MATRIXPORTAL_S3"
#endif

// HUB75 connector pinout built into the back of the Adafruit Matrix Portal S3
// (CircuitPython / Internet Display, product 5778).
// Matches the Adafruit Protomatter mapping; no external ESP32 wiring is needed.
// https://learn.adafruit.com/adafruit-matrixportal-s3
// https://github.com/adafruit/Adafruit_Protomatter

#define MP_S3_R1 42
#define MP_S3_G1 41
#define MP_S3_B1 40
#define MP_S3_R2 38
#define MP_S3_G2 39
#define MP_S3_B2 37
#define MP_S3_A 45
#define MP_S3_B 36
#define MP_S3_C 48
#define MP_S3_D 35
#define MP_S3_E 21
#define MP_S3_LAT 47
#define MP_S3_OE 14
#define MP_S3_CLK 2
