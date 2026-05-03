#pragma once

#include <cstdint>

struct UiModel;

// Pilote HUB75 pour Matrix Portal S3 (chaîne horizontale), aligné sur
// ProtoTracer MatrixPortalS3Controller::Initialize (sans miroir Display).

class Hub75MatrixPortal {
public:
  bool begin(uint8_t initial_brightness8 = 128);
  bool ok() const { return panel_ != nullptr; }

  // Bandes R / V / B sur les tiers de largeur (boot ProtoTracer).
  void run_boot_rgb_horizontal_thirds(uint32_t hold_ms = 1200);

  // Écran entier une couleur (test alim / pixels).
  void fill_screen_rgb888(uint8_t r, uint8_t g, uint8_t b);

  // Texte via Adafruit_GFX (police bitmap par défaut).
  void draw_hello_world();
  void draw_ui(const UiModel& model);

  void clear_screen();
  void set_brightness8(uint8_t value);
  void flip_dma_buffer();

private:
  void* panel_ = nullptr; // MatrixPanel_I2S_DMA* lorsque BOARD_MATRIXPORTAL_S3
};
