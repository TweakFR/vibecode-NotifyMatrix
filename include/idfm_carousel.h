#pragma once

#include <stddef.h>

/// Une entrée du carrousel PRIM (MonitoringRef / LineRef au format STIF).
struct IdfmCarouselSlot {
  const char* monitoring_ref;
  const char* line_ref;
  const char* line_code;
  const char* label;
};

extern const IdfmCarouselSlot kIdfmCarouselSlots[];
extern const size_t kIdfmCarouselCount;
