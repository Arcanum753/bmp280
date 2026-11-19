#include "core_web/FSWebServerLib.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "module_bmp280.h"

#include "core_terminal/core_terminal.h"

// ============================================================
// Терминальные команды module_bmp280
// ============================================================

static const char* const kBmpCommands[] = {
    "show", nullptr
};

static const char kBmpHelp[] PROGMEM =
    "bmp - BMP280 temperature/pressure sensor\r\n"
    "  bmp        show availability, address and readings\r\n"
    "  bmp show   same as 'bmp'\r\n"
    "  bmp help   this help\r\n";

void CLASS_MODULE_BMP280::terminalRegister() {
    TerminalRegisterModule("module_bmp280", "bmp", "BMP280 temperature/pressure sensor",
                           &CLASS_MODULE_BMP280::s_terminalCmd,
                           kBmpCommands, kBmpHelp, false);
}

void CLASS_MODULE_BMP280::s_terminalCmd() {
    char* sub = term.getNext();
    if (sub == nullptr) {
        s_cmdShow();
        return;
    }
    if (strcmp(sub, "help") == 0) {
        TerminalPrintText(kBmpHelp);
        return;
    }

    if (strcmp(sub, "show") == 0) {
        s_cmdShow();
        return;
    }

    Serial.printf("Unknown bmp subcommand '%s'. Type 'bmp help'.\r\n", sub);
}

void CLASS_MODULE_BMP280::s_cmdShow() {
    if (term.getNext() != nullptr) {
        Serial.println("Usage: bmp [show]");
        return;
    }

    Serial.printf("\r\nBMP280 @0x%02X: %s\r\n",
                  module_bmp280._addr,
                  module_bmp280._available ? "available" : "not available");

    if (!module_bmp280._available) { return; }

    Serial.printf("Chip ID:     0x%02X\r\n", module_bmp280._chipId);
    Serial.printf("Temperature: %.2f C\r\n", module_bmp280._temperature);
    Serial.printf("Pressure:    %.2f hPa\r\n", module_bmp280._pressure);
}
