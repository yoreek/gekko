#if defined(ARDUINO) && !defined(UNIT_TEST)

#include "core/App.h"
#include "debug/Debug.h"

#include <Arduino.h>
#include <esp_rom_sys.h>
#include <esp_system.h>

namespace {
ewfm::App app;

const char* resetReasonToString(const esp_reset_reason_t reason) {
    switch (reason) {
    case ESP_RST_UNKNOWN:
        return "UNKNOWN";
    case ESP_RST_POWERON:
        return "POWERON";
    case ESP_RST_EXT:
        return "EXT";
    case ESP_RST_SW:
        return "SW";
    case ESP_RST_PANIC:
        return "PANIC";
    case ESP_RST_INT_WDT:
        return "INT_WDT";
    case ESP_RST_TASK_WDT:
        return "TASK_WDT";
    case ESP_RST_WDT:
        return "WDT";
    case ESP_RST_DEEPSLEEP:
        return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:
        return "BROWNOUT";
    case ESP_RST_SDIO:
        return "SDIO";
    default:
        return "OTHER";
    }
}
} // namespace

void setup() {
    EWFM_DEBUG_BEGIN();
    esp_rom_printf("BOOT reset reason=%s\n", resetReasonToString(esp_reset_reason()));
    esp_rom_printf("BOOT setup begin\n");
    app.begin();
    esp_rom_printf("BOOT setup end\n");
}

void loop() {
    app.tick();
}

#endif
