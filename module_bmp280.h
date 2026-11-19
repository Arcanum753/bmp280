#ifndef _MODULE_BMP280_h
#define _MODULE_BMP280_h

#include "main.h"

#include "mod_context.h"

#ifdef DEBUG_BMP280
#define DEBUGBMP280(...) DBG_MOD("[M_BMP280] ", __VA_ARGS__)
#else
#define DEBUGBMP280(...)
#endif

#include "module_bmp280_types.h"

// ============================================================
// module_bmp280 — датчик температуры и давления BMP280 (I2C).
// Публикует на шину только доступность и значения; настроек нет.
// ============================================================
class CLASS_MODULE_BMP280 {
public:
    CLASS_MODULE_BMP280();
    void begin();
    void begin(ModContext& ctx);
    void web_Init();
    void register_resources();

    // ===== Публичное API =====
    void pollStep();
    bool isAvailable() const;
    float getTemperature() const;
    float getPressure() const;
    uint8_t getAddr() const;
    uint8_t getChipId() const;

private:
    // Терминальные команды: статические методы-члены, поэтому имеют
    // прямой доступ к приватному состоянию модуля (см. _terminal.cpp).
    void terminalRegister();
    static void s_terminalCmd();
    static void s_cmdShow();

    // Версионные методы
    String getVersionStr();
    String getGeneratedTime();
    String getCommitDateStr();
    void html_ver_get(AsyncWebServerRequest *request);

    // Веб-обработчики
    void handleInfo(AsyncWebServerRequest *request);
    void handleRead(AsyncWebServerRequest *request);
    void appendValues(String &values);

    // I2C и расчёт
    bool detect();
    bool initChip();
    bool readCalibration();
    bool readRaw(int32_t &rawTemp, int32_t &rawPress);
    bool readRegs(uint8_t reg, uint8_t *buf, uint8_t len);
    bool writeReg(uint8_t reg, uint8_t val);
    int32_t compensateTemp(int32_t adcTemp);
    int32_t compensatePressure(int32_t adcPress);
    void publish(bool available, float temperature, float pressure);

    // EERTOS-задача
    static void pollTask();

protected:
    bool           _wireStarted;
    bool           _available;
    bool           _calibValid;
    uint8_t        _addr;
    uint8_t        _chipId;
    float          _temperature;
    float          _pressure;
    int32_t        _tFine;
    strBmp280Calib _calib;
};

extern CLASS_MODULE_BMP280 module_bmp280;

#endif // _MODULE_BMP280_h
