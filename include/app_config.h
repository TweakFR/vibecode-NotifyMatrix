#pragma once

// Géométrie : 2 modules 64×32 côte à côte (chaîne HUB75 horizontale) = 128×32
#ifndef PANEL_MODULE_WIDTH
#define PANEL_MODULE_WIDTH 64
#endif
#ifndef PANEL_MODULE_HEIGHT
#define PANEL_MODULE_HEIGHT 32
#endif
#ifndef PANEL_CHAIN_LENGTH
#define PANEL_CHAIN_LENGTH 2
#endif

#define DISPLAY_TOTAL_WIDTH (PANEL_MODULE_WIDTH * PANEL_CHAIN_LENGTH)
#define DISPLAY_TOTAL_HEIGHT PANEL_MODULE_HEIGHT

// Disposition 128×32 : gauche 96 px = heure / notification MQTT, droite 32 px = bus
#define ZONE_TIME_X 0
#define ZONE_TIME_W 96
#define ZONE_BUS_X ZONE_TIME_W
#define ZONE_BUS_W (DISPLAY_TOTAL_WIDTH - ZONE_TIME_W)

// Décalage vertical du HUD (heure + zone bus + séparateurs), en % de DISPLAY_TOTAL_HEIGHT
// (ex. 2 → ~1 px sur 32 px de hauteur) pour mieux centrer visuellement sur le panneau.
#ifndef HUD_VERTICAL_OFFSET_PERCENT
#define HUD_VERTICAL_OFFSET_PERCENT 2
#endif

// Animation MQTT : heure + bus remontent, bande basse + défilement (fluidité = UI_REFRESH_MS bas)
#ifndef NOTIFY_SHRINK_PX
#define NOTIFY_SHRINK_PX 7
#endif
#ifndef NOTIFY_SHRINK_MS
#define NOTIFY_SHRINK_MS 520
#endif
#ifndef NOTIFY_UNSHRINK_MS
#define NOTIFY_UNSHRINK_MS 520
#endif
// Défilement droite → gauche (px/s) — plus bas = plus lent
#ifndef NOTIFY_SCROLL_PPS
#define NOTIFY_SCROLL_PPS 30
#endif

// MQTT — sujets par défaut (modifiables dans secrets ou ici)
#ifndef MQTT_TOPIC_NOTIFY
#define MQTT_TOPIC_NOTIFY "notifymatrix/notify"
#endif
#ifndef MQTT_TOPIC_BUS
#define MQTT_TOPIC_BUS "notifymatrix/bus/next"
#endif

#ifndef MQTT_CLIENT_ID
#define MQTT_CLIENT_ID "notifymatrix-s3"
#endif

// Durée d’affichage d’une notification (ms) avant retour à l’heure
#ifndef NOTIFY_DISPLAY_MS
#define NOTIFY_DISPLAY_MS 15000
#endif

// Rafraîchissement affichage (ms) — 16 ≈ 60 Hz pour animations fluides
#ifndef UI_REFRESH_MS
#define UI_REFRESH_MS 16
#endif

// Hors animation MQTT : ne redessine la matrice que si l’UI a changé, au plus toutes N ms (anti-scintillement).
#ifndef UI_IDLE_REFRESH_MS
#define UI_IDLE_REFRESH_MS 250
#endif

// Connexion WiFi initiale (ms)
#ifndef WIFI_CONNECT_TIMEOUT_MS
#define WIFI_CONNECT_TIMEOUT_MS 15000
#endif

// Après réception PRIM pour le slot **suivant** : garder encore N ms l’affichage du slot **actuel**,
// puis seulement afficher le suivant (on ne montre jamais la ligne suivante avant sa réponse API).
#ifndef IDFM_HOLD_AFTER_PREFETCH_MS
#define IDFM_HOLD_AFTER_PREFETCH_MS 3000u
#endif
// Après changement de ligne (carrousel) : garder l’heure + libellé + «--» visibles au moins N ms
// avant l’appel API bloquant (l’heure est aussi poussée sur la matrice juste avant la requête).
#ifndef IDFM_LINE_CHANGE_CLOCK_DWELL_MS
#define IDFM_LINE_CHANGE_CLOCK_DWELL_MS 1500u
#endif

// Configuration IDFM / PRIM (surchargeable dans secrets.h)
#ifndef IDFM_API_KEY
#define IDFM_API_KEY ""
#endif
#ifndef IDFM_MONITORING_REF
#define IDFM_MONITORING_REF ""
#endif
#ifndef IDFM_LINE_REF
#define IDFM_LINE_REF ""
#endif
#ifndef IDFM_LINE_LABEL
#define IDFM_LINE_LABEL DEFAULT_BUS_LINE_LABEL
#endif
#ifndef IDFM_LINE_CODE
#define IDFM_LINE_CODE ""
#endif

// Tailles maximales des textes affichés / reçus.
#ifndef NOTIFICATION_TEXT_MAX
#define NOTIFICATION_TEXT_MAX 64
#endif
#ifndef BUS_LABEL_MAX
#define BUS_LABEL_MAX 12
#endif
#ifndef BUS_STATUS_MAX
#define BUS_STATUS_MAX 8
#endif

// Fuseau Europe/Paris (configTzTime) : UTC+1 hiver, UTC+2 été — offsets explicites pour newlib
#ifndef TZ_POSIX
#define TZ_POSIX "CET-1CEST-2,M3.5.0/2,M10.5.0/3"
#endif

// Libellé ligne (zone 32 px) — ex. 2225 (shortName bus)
#ifndef DEFAULT_BUS_LINE_LABEL
#define DEFAULT_BUS_LINE_LABEL "2225"
#endif

// Driver puce colonnes HUB75 : même défaut que ProtoTracer (MatrixPortalS3Controller) = SHIFTREG
// (constructeur HUB75_I2S_CFG à 4 arguments, sans toucher à driver).
// Si les matrices restent noires avec le défaut mais ProtoTracer les allume : laisser SHIFTREG.
// Matrices Adafruit 64×32 FM6126A / ICN2038S : build_flags = -D PANEL_HUB75_DRIVER_FM6126A=1
// (ou -D PANEL_HUB75_DRIVER_ICN2038S=1 selon le module).
// Optionnel : lat blanking (ex. 2) si artefacts — -D PANEL_HUB75_LAT_BLANKING=2
#ifndef PANEL_HUB75_LAT_BLANKING
#define PANEL_HUB75_LAT_BLANKING 0
#endif

// Durée d’affichage de chaque étape du test couleurs au boot (ms). 0 = désactiver le test.
#ifndef HUB75_SELFTEST_STEP_MS
#define HUB75_SELFTEST_STEP_MS 400
#endif

// Panneaux 64×32 1/16 sans ligne d’adresse E : essayer -DPANEL_HUB75_DISABLE_ROW_E=1 si le self-test reste noir.
#ifndef PANEL_HUB75_DISABLE_ROW_E
#define PANEL_HUB75_DISABLE_ROW_E 0
#endif
