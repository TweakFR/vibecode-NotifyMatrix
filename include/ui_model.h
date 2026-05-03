#pragma once

#include "app_config.h"

#include <cstdint>

enum class BusState : uint8_t {
  Idle,
  Loading,
  Ready,
  Error,
};

struct UiModel {
  char time_text[8] = "--:--";
  char notification[NOTIFICATION_TEXT_MAX] = "";
  bool notification_active = false;
  /// 0 … NOTIFY_SHRINK_PX : hauteur cédée en bas pour la bande de défilement
  uint8_t notify_shrink_px = 0;
  /// Position gauche du texte de notification (défilement droite → gauche)
  int16_t notify_scroll_x = 0;
  /// Afficher le texte vert dans la bande (phase défilement uniquement)
  bool notify_scroll_visible = false;

  char bus_line[BUS_LABEL_MAX] = DEFAULT_BUS_LINE_LABEL;
  char bus_text[BUS_STATUS_MAX] = "--";
  /// Minutes jusqu’au passage (IDFM), ≥0 si Ready et donnée valide ; −1 sinon
  int16_t bus_eta_minutes = -1;
  BusState bus_state = BusState::Idle;

  bool wifi_connected = false;
  bool mqtt_connected = false;
  bool time_synced = false;
};
