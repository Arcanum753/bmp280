#include "core_web/FSWebServerLib.h"

#include "core_state/core_state.h"

#include "module_bmp280.h"

#include <Wire.h>

// ============================================================
// I2C — низкоуровневые операции
// ============================================================

bool CLASS_MODULE_BMP280::readRegs(uint8_t reg, uint8_t *buf, uint8_t len) {
    if (_addr == 0) { return false; }

    Wire.beginTransmission(_addr);
    Wire.write(reg);
    // Repeated START: чтение начинается без STOP.
    if (Wire.endTransmission(false) != 0) { return false; }
    if (Wire.requestFrom((int)_addr, (int)len) != (int)len) { return false; }

    for (uint8_t i = 0; i < len; i++) {
        if (Wire.available() < 1) { return false; }
        buf[i] = (uint8_t)Wire.read();
    }
    return true;
}

bool CLASS_MODULE_BMP280::writeReg(uint8_t reg, uint8_t val) {
    if (_addr == 0) { return false; }

    Wire.beginTransmission(_addr);
    Wire.write(reg);
    Wire.write(val);
    return (Wire.endTransmission() == 0);
}

// ============================================================
// Поиск и инициализация датчика
// ============================================================

bool CLASS_MODULE_BMP280::detect() {
    static const uint8_t kAddrs[2] = {
        BMP280_I2C_ADDR_PRIMARY, BMP280_I2C_ADDR_SECONDARY
    };

    uint8_t oldAddr = _addr;
    uint8_t otherChip = 0;

    for (uint8_t i = 0; i < 2; i++) {
        uint8_t a = kAddrs[i];

        Wire.beginTransmission(a);
        if (Wire.endTransmission() != 0) { continue; }

        _addr = a;
        uint8_t id = 0;
        if (!readRegs(BMP280_REG_CHIP_ID, &id, 1)) {
            _addr = 0;
            continue;
        }

        if (id == BMP280_CHIP_ID) {
            _chipId = id;
            if (oldAddr != a) { _calibValid = false; }
            DEBUGBMP280("BMP280 found at 0x%02X\r\n", a);
            return true;
        }

        otherChip = id;
        _addr = 0;
    }

    if (otherChip == BMP280_CHIP_ID_BME280) {
        DEBUGBMP280("BME280 at 0x%02X is not supported\r\n", otherChip);
    }
    _chipId = 0;
    _calibValid = false;
    return false;
}

bool CLASS_MODULE_BMP280::readCalibration() {
    uint8_t b[26];
    if (!readRegs(BMP280_REG_CALIB, b, 26)) {
        _calibValid = false;
        return false;
    }

    _calib.dig_T1 = (uint16_t)b[0]  | ((uint16_t)b[1]  << 8);
    _calib.dig_T2 = (int16_t)((uint16_t)b[2]  | ((uint16_t)b[3]  << 8));
    _calib.dig_T3 = (int16_t)((uint16_t)b[4]  | ((uint16_t)b[5]  << 8));
    _calib.dig_P1 = (uint16_t)b[6]  | ((uint16_t)b[7]  << 8);
    _calib.dig_P2 = (int16_t)((uint16_t)b[8]  | ((uint16_t)b[9]  << 8));
    _calib.dig_P3 = (int16_t)((uint16_t)b[10] | ((uint16_t)b[11] << 8));
    _calib.dig_P4 = (int16_t)((uint16_t)b[12] | ((uint16_t)b[13] << 8));
    _calib.dig_P5 = (int16_t)((uint16_t)b[14] | ((uint16_t)b[15] << 8));
    _calib.dig_P6 = (int16_t)((uint16_t)b[16] | ((uint16_t)b[17] << 8));
    _calib.dig_P7 = (int16_t)((uint16_t)b[18] | ((uint16_t)b[19] << 8));
    _calib.dig_P8 = (int16_t)((uint16_t)b[20] | ((uint16_t)b[21] << 8));
    _calib.dig_P9 = (int16_t)((uint16_t)b[22] | ((uint16_t)b[23] << 8));

    // dig_P1 == 0 приводит к делению на ноль в компенсации давления.
    _calibValid = (_calib.dig_P1 != 0);
    return _calibValid;
}

bool CLASS_MODULE_BMP280::initChip() {
    if (!readCalibration()) {
        DEBUGBMP280("calibration read failed\r\n");
        return false;
    }

    if (!writeReg(BMP280_REG_CTRL_MEAS, BMP280_CTRL_MEAS_NORMAL)) {
        DEBUGBMP280("ctrl_meas write failed\r\n");
        return false;
    }
    if (!writeReg(BMP280_REG_CONFIG, BMP280_CONFIG_FILTER_X4)) {
        DEBUGBMP280("config write failed\r\n");
        return false;
    }

    return true;
}

// ============================================================
// Чтение и компенсация
// ============================================================

bool CLASS_MODULE_BMP280::readRaw(int32_t &rawTemp, int32_t &rawPress) {
    uint8_t b[6];
    if (!readRegs(BMP280_REG_DATA, b, 6)) { return false; }

    // 0xF7..0xF9 — давление, 0xFA..0xFC — температура (20 бит).
    rawPress = ((int32_t)b[0] << 12) | ((int32_t)b[1] << 4) | ((int32_t)b[2] >> 4);
    rawTemp  = ((int32_t)b[3] << 12) | ((int32_t)b[4] << 4) | ((int32_t)b[5] >> 4);
    return true;
}

// Компенсация по даташиту Bosch; результат — 0.01 °C.
int32_t CLASS_MODULE_BMP280::compensateTemp(int32_t adcTemp) {
    int32_t var1 = ((((adcTemp >> 3) - ((int32_t)_calib.dig_T1 << 1))) *
                    ((int32_t)_calib.dig_T2)) >> 11;
    int32_t var2 = (((((adcTemp >> 4) - ((int32_t)_calib.dig_T1)) *
                       ((adcTemp >> 4) - ((int32_t)_calib.dig_T1))) >> 12) *
                    ((int32_t)_calib.dig_T3)) >> 14;

    _tFine = var1 + var2;
    return (_tFine * 5 + 128) >> 8;
}

// Компенсация по даташиту Bosch; результат — Па.
int32_t CLASS_MODULE_BMP280::compensatePressure(int32_t adcPress) {
    int64_t var1;
    int64_t var2;
    int64_t p;

    var1 = ((int64_t)_tFine) - 128000;
    var2 = var1 * var1 * (int64_t)_calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)_calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)_calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)_calib.dig_P3) >> 8) +
           ((var1 * (int64_t)_calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1) * ((int64_t)_calib.dig_P1)) >> 33;

    if (var1 == 0) { return 0; }

    p = 1048576 - adcPress;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)_calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)_calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)_calib.dig_P7) << 4);

    return (int32_t)p;
}

void CLASS_MODULE_BMP280::publish(bool available, float temperature, float pressure) {
    _available = available;
    if (available) {
        _temperature = temperature;
        _pressure    = pressure;
    } else {
        _temperature = 0.0f;
        _pressure    = 0.0f;
    }

    core_state.signal("bmp280.available",   BusValue::bo(_available));
    core_state.signal("bmp280.temperature", BusValue::f32(_temperature));
    core_state.signal("bmp280.pressure",    BusValue::f32(_pressure));
}

// ============================================================
// Опрос датчика
// ============================================================

void CLASS_MODULE_BMP280::pollStep() {
    if (!_wireStarted) { _wireStarted = true; Wire.begin(); }

    // Датчик потерян или ещё не найден — пробуем найти и настроить.
    if (_addr == 0) {
        if (!detect() || !initChip()) {
            _addr     = 0;
            _chipId   = 0;
            publish(false, 0.0f, 0.0f);
            return;
        }
    } else if (!_calibValid) {
        if (!initChip()) {
            _addr   = 0;
            _chipId = 0;
            publish(false, 0.0f, 0.0f);
            return;
        }
    }

    int32_t rawTemp  = 0;
    int32_t rawPress = 0;
    if (!readRaw(rawTemp, rawPress)) {
        DEBUGBMP280("read failed, re-detect on next poll\r\n");
        _addr       = 0;
        _chipId     = 0;
        _calibValid = false;
        publish(false, 0.0f, 0.0f);
        return;
    }

    int32_t t = compensateTemp(rawTemp);
    int32_t p = compensatePressure(rawPress);

    if (p <= 0) {
        DEBUGBMP280("pressure compensation failed\r\n");
        _addr       = 0;
        _chipId     = 0;
        _calibValid = false;
        publish(false, 0.0f, 0.0f);
        return;
    }

    // Bosch integer pressure compensation returns Q24.8 (Pa * 256);
    // переводим в Па и затем в гПа.
    publish(true, (float)t / 100.0f, (float)p / 25600.0f);
}

void CLASS_MODULE_BMP280::pollTask() {
    module_bmp280.pollStep();
}
