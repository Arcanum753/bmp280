#include "core_web/FSWebServerLib.h"

#include "core_state/core_state.h"
#include "core_task/core_task.h"
#include "core_terminal/core_terminal.h"
#include "core_terminal/ErriezSerialTerminal.h"

#include "module_bmp280.h"
#include "module_bmp280_version.h"
#include "core_sys/eertos.h"

#include <Wire.h>
#include <stdio.h>

CLASS_MODULE_BMP280 module_bmp280;

CLASS_MODULE_BMP280::CLASS_MODULE_BMP280() {
    _wireStarted  = false;
    _available    = false;
    _calibValid   = false;
    _addr         = 0;
    _chipId       = 0;
    _temperature  = 0.0f;
    _pressure     = 0.0f;
    _tFine        = 0;

    _calib.dig_T1 = 0;
    _calib.dig_T2 = 0;
    _calib.dig_T3 = 0;
    _calib.dig_P1 = 0;
    _calib.dig_P2 = 0;
    _calib.dig_P3 = 0;
    _calib.dig_P4 = 0;
    _calib.dig_P5 = 0;
    _calib.dig_P6 = 0;
    _calib.dig_P7 = 0;
    _calib.dig_P8 = 0;
    _calib.dig_P9 = 0;
}

// ============================================================
// begin()
// ============================================================
void CLASS_MODULE_BMP280::begin() {
    DEBUGBMP280("%s\r\n", __FUNCTION__);

    if (!_wireStarted) { _wireStarted = true; Wire.begin(); }

    _addr       = 0;
    _chipId     = 0;
    _available  = false;
    _calibValid = false;

    if (detect()) {
        _available = initChip();
    } else {
        DEBUGBMP280("BMP280: not found\r\n");
    }

    terminalRegister();

    // Периодический опрос: если датчик не найден, pollStep повторяет detect().
    core_task.every("bmp280_poll", pollTask, BMP280_POLL_MS, true);
}

void CLASS_MODULE_BMP280::begin(ModContext& ctx) {
    (void)ctx;
    begin();
}

// ============================================================
// web_Init()
// ============================================================
void CLASS_MODULE_BMP280::web_Init() {
    DEBUGBMP280("%s\r\n", __FUNCTION__);

    // AJAX — статическая информация и текущие показания
    ESPHTTPServer.on("/bmp280/info", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!ESPHTTPServer.checkAuth(request)) { return request->requestAuthentication(); }
        this->handleInfo(request);
    });

    // AJAX — только показания (частый опрос без перезаписи формы)
    ESPHTTPServer.on("/bmp280/read", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!ESPHTTPServer.checkAuth(request)) { return request->requestAuthentication(); }
        this->handleRead(request);
    });

    // Версия модуля
    ESPHTTPServer.on("/bmp280/ver", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->html_ver_get(request);
    });
}

// ============================================================
// register_resources() — публикация ресурсов на шине core_state
// ============================================================
void CLASS_MODULE_BMP280::register_resources() {
    DEBUGBMP280("%s\r\n", __FUNCTION__);

    core_state.regState("available",   BusValue::BOOL, "BMP280 present on I2C bus", false);
    core_state.regState("temperature", BusValue::F32,  "temperature (C)",           false);
    core_state.regState("pressure",    BusValue::F32,  "pressure (hPa)",            false);
}

// ============================================================
// Веб-обработчики
// ============================================================
void CLASS_MODULE_BMP280::appendValues(String &values) {
    if (_available) {
        char tbuf[16];
        char pbuf[16];
        snprintf(tbuf, sizeof(tbuf), "%.2f", _temperature);
        snprintf(pbuf, sizeof(pbuf), "%.2f", _pressure);
        values += "bmp_temp|"  + String(tbuf) + " °C|div\n";
        values += "bmp_press|" + String(pbuf) + " hPa|div\n";
    } else {
        values += "bmp_temp|--|div\n";
        values += "bmp_press|--|div\n";
    }
}

void CLASS_MODULE_BMP280::handleInfo(AsyncWebServerRequest *request) {
    DEBUGBMP280("%s\r\n", __FUNCTION__);

    String values = "";
    char buf[16];

    values += "bmp_available|" + String(_available ? "available" : "not available") + "|div\n";

    snprintf(buf, sizeof(buf), "0x%02X", _addr);
    values += "bmp_addr|" + String(buf) + "|div\n";

    snprintf(buf, sizeof(buf), "0x%02X", _chipId);
    values += "bmp_chip|" + String(buf) + "|div\n";

    appendValues(values);
    request->send(200, "text/plain", values);
}

void CLASS_MODULE_BMP280::handleRead(AsyncWebServerRequest *request) {
    String values = "";
    appendValues(values);
    request->send(200, "text/plain", values);
}

// ============================================================
// Версионные методы
// ============================================================
String CLASS_MODULE_BMP280::getVersionStr() {
    return String(MODULE_BMP280_VERSION);
}

String CLASS_MODULE_BMP280::getGeneratedTime() {
    return String(MODULE_BMP280_GENERATED_TIME);
}

String CLASS_MODULE_BMP280::getCommitDateStr() {
    return String(MODULE_BMP280_COMMIT_DATE_STR);
}

void CLASS_MODULE_BMP280::html_ver_get(AsyncWebServerRequest *request) {
    DEBUGBMP280("%s\r\n", __FUNCTION__);
    String values = "";
    values += "bmpversion|" + getVersionStr()    + "|div\n";
    values += "bmpgentime|" + getGeneratedTime() + "|div\n";
    values += "bmpgendate|" + getCommitDateStr() + "|div\n";
    request->send(200, "text/plain", values);
}

// ============================================================
// Публичное API
// ============================================================
bool CLASS_MODULE_BMP280::isAvailable() const { return _available; }
float CLASS_MODULE_BMP280::getTemperature() const { return _temperature; }
float CLASS_MODULE_BMP280::getPressure() const { return _pressure; }
uint8_t CLASS_MODULE_BMP280::getAddr() const { return _addr; }
uint8_t CLASS_MODULE_BMP280::getChipId() const { return _chipId; }
