/**
 * Runs before setup() and proves that the firmware image actually started.
 * Useful when the USB serial monitor is not ready yet.
 */
#include <esp_rom_sys.h>

extern "C" __attribute__((constructor(101))) void notifymatrix_firmware_ctor_marker()
{
  esp_rom_printf(
      "\r\n\r\n"
      "======== NOTIFYMATRIX FIRMWARE START (C++ ctor) ========\r\n"
      "If this appears in the USB/JTAG serial monitor, the firmware is running.\r\n"
      "========================================================\r\n\r\n");
}
