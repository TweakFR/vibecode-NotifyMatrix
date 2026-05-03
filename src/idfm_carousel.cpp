#include "idfm_carousel.h"

// Arrêts / lignes (IDs STIF pour PRIM). Dernier arrêt : lieu multimodal → StopArea SP (ajuster si besoin).
const IdfmCarouselSlot kIdfmCarouselSlots[] = {
    {"STIF:StopPoint:Q:15352:", "STIF:Line::C00628:", "C00628", "2225"},
    {"STIF:StopPoint:Q:15687:", "STIF:Line::C00627:", "C00627", "2223"},
    {"STIF:StopPoint:Q:15687:", "STIF:Line::C01879:", "C01879", "2224"},
    {"STIF:StopPoint:Q:15687:", "STIF:Line::C01639:", "C01639", "N141"},
    {"STIF:StopPoint:Q:15687:", "STIF:Line::C00625:", "C00625", "2221"},
    {"STIF:StopPoint:Q:15687:", "STIF:Line::C00634:", "C00634", "2220"},
    {"STIF:StopPoint:Q:15687:", "STIF:Line::C00629:", "C00629", "2226"},
    {"STIF:StopArea:SP:427872:", "STIF:Line::C01730:", "C01730", "P"},
};

const size_t kIdfmCarouselCount = sizeof(kIdfmCarouselSlots) / sizeof(kIdfmCarouselSlots[0]);
