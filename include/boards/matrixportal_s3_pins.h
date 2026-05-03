#pragma once

#if !defined(BOARD_MATRIXPORTAL_S3)
#error "Ce fichier cible uniquement BOARD_MATRIXPORTAL_S3"
#endif

// Brochage du connecteur HUB75 **intégré au dos** du Matrix Portal S3
// (Adafruit « CircuitPython / Internet Display », même carte — produit 5778).
// Aligné sur Adafruit Protomatter (pas de câblage externe à l’ESP32).
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
