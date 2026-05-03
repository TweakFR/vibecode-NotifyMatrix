#pragma once

// Les identifiants ne sont plus dans ce fichier : tout est dans un fichier .env à la racine.
//
// 1. Copier le modèle :  cp .env.example .env
// 2. Éditer .env (WIFI_*, MQTT_*, IDFM_*).
// 3. Compiler : PlatformIO exécute scripts/gen_secrets_header.py avant la compilation
//    et génère include/secrets_from_env.h (gitignoré).
//
// include/secrets.h inclut uniquement secrets_from_env.h.
//
// Test curl PRIM (même logique que le MCU) :
//   export IDFM_API_KEY='…'
//   export REF='STIF:StopPoint:Q:15352:'
//   curl -sS -G 'https://prim.iledefrance-mobilites.fr/marketplace/stop-monitoring' \
//     --data-urlencode "MonitoringRef=$REF" \
//     -H 'accept: application/json' \
//     -H "apikey: $IDFM_API_KEY" | jq .
