/**
 * S'exécute avant setup() : prouve que l'image firmware est bien lancée
 * (utile si USB série / moniteur n'est pas encore prêt).
 */
#include <esp_rom_sys.h>

extern "C" __attribute__((constructor(101))) void notifymatrix_firmware_ctor_marker()
{
  esp_rom_printf(
      "\r\n\r\n"
      "======== NOTIFYMATRIX FIRMWARE START (C++ ctor) ========\r\n"
      "Si vous voyez ceci dans le moniteur série USB/JTAG, le binaire tourne.\r\n"
      "========================================================\r\n\r\n");
}
