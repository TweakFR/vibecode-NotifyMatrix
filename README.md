# NotifyMatrix

Firmware pour **Adafruit Matrix Portal S3** et **deux panneaux HUB75 en chaîne horizontale** (souvent 64×32 chacun → **128×32** logiques). Le dépôt est découpé en **phase 1** (matériel uniquement) puis **phase 2** (réseau et UI).

## Phase 1 — Validation des deux matrices

Le firmware actuel ne fait **que** piloter le HUB75, sur le modèle de **ProtoTracer** [`MatrixPortalS3Controller::Initialize`](ProtoTracer/lib/ProtoTracer/Controller/MatrixPortalS3Controller.cpp) (broches, `HUB75_I2S_CFG`, `clkphase = false`, bandes RGB sur les tiers de largeur au boot). Il n’y a **pas** de WiFi, MQTT ni NTP.

**Comportement attendu**

1. Au démarrage : bandes **rouge / vert / bleu** sur le tiers gauche, central et droit de toute la largeur (128 px).
2. Ensuite : affichage fixe **moitié gauche rouge** (premier panneau, colonnes 0 à 63), **moitié droite verte** (colonnes 64 à 127). La boucle rafraîchit ce motif toutes les 2 s (swap DMA).

Si les couleurs sont inversées ou le découpage ne correspond pas au câblage, vérifier l’alimentation 5 V et les options de pilote dans `platformio.ini` (voir plus bas).

### Vérifier que le firmware s’exécute (sans regarder les matrices)

1. **`pio device monitor`** (115200) : au reset, une ligne **`NOTIFYMATRIX FIRMWARE START (C++ ctor)`** part de la ROM (`esp_rom_printf`) avant `setup()`, puis des messages `Serial` / `Serial.printf` (date de compilation, heap, etc.).
2. **LED D13** sur la carte (pas les HUB75) : clignotement ~250 ms en boucle = `loop()` tourne même si le HUB75 échoue.

Si vous n’avez **ni** texte série **ni** clignotement D13 alors que le moniteur affiche déjà les messages NotifyMatrix (ctor + `setup`), le problème peut être flash / partition / mauvaise carte ou USB.

### Le moniteur affiche `waiting for download` (ROM bootloader)

Si vous voyez seulement des lignes du type :

```text
ESP-ROM:esp32s3-20210327
rst:0x15 (USB_UART_CHIP_RESET),boot:0x0 (DOWNLOAD(USB/UART0))
waiting for download
```

alors l’ESP32-S3 est en **mode téléchargement** (ROM qui attend esptool). **Votre sketch ne tourne pas** — ce n’est pas un bug du printf ni des matrices.

À faire dans l’ordre :

1. **Ne pas** maintenir le bouton **BOOT** (sinon on reste en téléchargement).
2. **Un seul appui sur Reset** (sans BOOT) pour lancer le démarrage normal depuis la flash (firmware Arduino).
3. Si ça reste bloqué : débrancher l’USB quelques secondes, rebrancher, ouvrir le moniteur, puis **Reset** une fois.
4. Éviter d’enchaîner upload + moniteur sans regarder la sortie : après `Hard resetting…`, attendre ~1 s puis **Reset** physique si la première ligne reste `waiting for download`.

Tant que la puce reste **bloquée** en téléchargement (ROM qui ne fait que `waiting for download` à chaque reset, **sans** LED D13 au rythme du sketch), l’application ne démarre pas.

### La LED clignote mais le moniteur ne montre que `waiting for download`

Si la **LED D13** bat bien au rythme du firmware (~2 impulsions visibles par seconde avec le réglage actuel), **l’ESP exécute déjà votre sketch**. Dans ce cas, des lignes `waiting for download` en haut du terminal sont souvent des **restes d’une session précédente** (scroll vers le bas) ou la **sortie ROM** sur un canal USB différent de celle utilisée par `Serial`.

Sur Matrix Portal S3, il faut diriger **`Serial` vers l’USB CDC** au boot (comme dans ProtoTracer) : dans [platformio.ini](platformio.ini), les flags **`ARDUINO_USB_MODE=1`** et **`ARDUINO_USB_CDC_ON_BOOT=1`** sont activés. Reflashez après `pio run`, puis **effacez le terminal** (ou fermez/rouvrez le moniteur), **Reset** la carte, et vous devriez voir `NOTIFYMATRIX` / `======== NotifyMatrix`.

### `Disconnected ([Errno 5] Input/output error)` puis reconnexion

Ce message vient du **pilote USB** (le périphérique `/dev/ttyACM…` a disparu ou a été réinitialisé). Ça arrive souvent quand l’ESP redémarre ou ré-énumère. **Ce n’est pas un `printf` de votre sketch** et **ça ne prouve pas** que `setup()` / `loop()` tournent : tant que la ROM affiche `waiting for download`, c’est encore le **mode téléchargement**.

### Upload `SUCCESS` mais moniteur = `waiting for download` tout de suite

L’upload **écrit bien** le firmware en flash (`Hash of data verified`, `SUCCESS`). Ensuite esptool envoie un **reset** ; à l’ouverture du moniteur, un **`rst:0x15 (USB_UART_CHIP_RESET)`** peut encore faire repartir la puce en **téléchargement** (`boot:0x0`) au lieu du démarrage normal.

Procédure qui marche souvent sur S3 USB natif :

1. Laisser finir `pio run -t upload` (sans rouvrir le moniteur dans la même seconde si ça coince).
2. Lancer **`pio device monitor -p /dev/ttyACM2 -b 115200`** (adapter le port).
3. **Dès que le moniteur est connecté**, faire **un seul appui sur Reset** sur la carte (**sans** maintenir BOOT).
4. Si besoin : fermer le moniteur, **attendre 2 s**, rouvrir le moniteur, puis **Reset** à nouveau.

Tant que vous ne voyez **pas** au moins une ligne du type `======== NotifyMatrix` ou `[loop] alive`, considérez que le **sketch n’est pas encore la sortie que vous regardez** (souvent ROM `waiting for download` au lieu de l’app).

### Prérequis et compilation

- [PlatformIO](https://platformio.org/) (CLI ou extension VS Code)
- Câble USB-C données + alim 5 V adaptée aux matrices

```bash
pio run -t upload
pio device monitor
```

(115200 bauds — déjà dans `platformio.ini`.)

### Fichiers utiles (phase 1)

| Fichier | Rôle |
|---------|------|
| [platformio.ini](platformio.ini) | Environnement `matrixportal_s3`, flags `BOARD_MATRIXPORTAL_S3`, `SPIRAM_DMA_BUFFER` |
| [include/app_config.h](include/app_config.h) | Largeur/hauteur d’un module, `PANEL_CHAIN_LENGTH`, constantes réservées phase 2 |
| [include/boards/matrixportal_s3_pins.h](include/boards/matrixportal_s3_pins.h) | Brochage HUB75 intégré au Matrix Portal S3 |
| [include/hub75_matrixportal.h](include/hub75_matrixportal.h) | API `Hub75MatrixPortal` |
| [src/hub75_matrixportal.cpp](src/hub75_matrixportal.cpp) | Init + tests visuels |
| [src/boot_marker.cpp](src/boot_marker.cpp) | Message ROM très tôt (`esp_rom_printf`) pour confirmer le démarrage du binaire |
| [src/main.cpp](src/main.cpp) | Point d’entrée : série + LED D13 + HUB75 |

### Référence ProtoTracer

Le sous-dossier **ProtoTracer** sert de **référence locale** (projet qui fonctionne déjà sur la même carte). NotifyMatrix ne l’embarque pas en bibliothèque : on réutilise seulement la logique d’initialisation DMA, pas le rendu Protogen (miroirs dans `Display()`).

---

## Phase 2 — Feuille de route (non implémentée dans le firmware actuel)

| Zone (64 px de large) | Comportement prévu |
|------------------------|--------------------|
| Gauche | Heure locale (NTP + fuseau), remplacée temporairement par une **notification MQTT** |
| Droite | **Minutes** avant le prochain bus d’une ligne (données publiées en MQTT, ex. JSON) |

L’intégration avec un opérateur de transport (IDFM, API locale, etc.) se fait **en amont** (script, Node-RED, Home Assistant…) qui publie sur un sujet convenu. Les sujets et durées par défaut restent décrits dans [include/app_config.h](include/app_config.h).

Secrets WiFi / MQTT : copier [include/secrets.example.h](include/secrets.example.h) vers `include/secrets.h` (non versionné). Depuis l’ESP32, le broker MQTT doit être une **IP LAN** (pas `localhost`).

Dépendances prévues pour la phase 2 : Adafruit GFX, PubSubClient, ArduinoJson (à rajouter dans `lib_deps` quand le code sera réintroduit).

---

## Upload USB (Matrix Portal S3)

### Double-clic Reset = mode UF2 (disque), pas le port série

Le double-clic sur **Reset** monte le volume **`MATRXS3BOOT`** : souvent **plus de `/dev/ttyACM0`** pour esptool. Un **seul** Reset ou rebrancher l’USB ramène le firmware / le port série.

### Mode ROM (téléchargement série) pour `pio run -t upload`

Le `platformio.ini` utilise `board_upload.before_reset = no_reset` : passer en bootloader ROM :

1. Brancher l’USB (câble **données**).
2. **Maintenir BOOT**.
3. **Appuyer puis relâcher Reset** en gardant BOOT.
4. **Relâcher BOOT**.
5. Lancer `pio run -t upload` tout de suite.

[Voir Adafruit — Matrix Portal S3](https://learn.adafruit.com/adafruit-matrixportal-s3).

**Port** : `upload_port = /dev/ttyACM0` par défaut ; adapter avec `pio device list` (`ttyACM1`, etc.). Éviter `/dev/ttyS0` (série carte mère). Règles udev PlatformIO : [documentation](https://docs.platformio.org/en/latest/core/installation/udev-rules.html).

**PlatformIO / esptool** : avec `use_1200bps_touch = false`, garder `board_upload.wait_for_upload_port = false` pour ne pas bloquer l’upload sur `ttyACM0`.

**DTR/RTS** : `monitor_rts = 0`, `monitor_dtr = 0` pour limiter les resets fantômes à l’ouverture du moniteur série.

### Flash complète (optionnel)

Pour effacer toute la flash avant upload (bootloader, partitions, TinyUF2, firmware), réactiver dans [platformio.ini](platformio.ini) :

```ini
extra_scripts = post:scripts/full_flash_upload.py
```

(voir [scripts/full_flash_upload.py](scripts/full_flash_upload.py)). Plus long ; efface aussi la NVS.

---

## Dépannage : matrices noires alors que l’upload réussit

- **Alimentation 5 V** et câbles HUB75 (sortie carte → entrée du premier panneau).
- **Pilote colonnes** : par défaut **SHIFTREG** (comme ProtoTracer). Panneaux Adafruit 64×32 souvent **FM6126A** ou **ICN2038S** : ajouter dans `build_flags` respectivement `-D PANEL_HUB75_DRIVER_FM6126A=1` ou `-D PANEL_HUB75_DRIVER_ICN2038S=1`, et éventuellement `-D PANEL_HUB75_LAT_BLANKING=2` si artefacts.
- **Pas de broche E** sur des 64×32 1/16 : essayer `-DPANEL_HUB75_DISABLE_ROW_E=1` (voir [include/app_config.h](include/app_config.h)).

La bibliothèque [ESP32 HUB75 DMA](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA) signale que sur Matrix Portal S3 le DMA peut interagir avec le Wi‑Fi (EMI) : en phase 2, en cas de déconnexions, réduire la profondeur de couleur ou consulter la doc du dépôt.

---

## Portabilité

- Nouvelle carte : nouvel `[env:...]` dans `platformio.ini`, defines `BOARD_*`, fichier `*_pins.h`, et une implémentation de la même surface d’API que `Hub75MatrixPortal` (ou un autre backend graphique).

## Licence

À définir.
