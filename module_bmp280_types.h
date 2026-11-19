#ifndef _MODULE_BMP280_TYPES_h
#define _MODULE_BMP280_TYPES_h

// ============================================================
// module_bmp280_types.h — адреса, регистры, константы и калибровка BMP280.
// Реализация: module_bmp280.cpp (шаблон) и module_bmp280_engine.cpp
// (I2C, компенсация, периодический опрос).
// ============================================================

#include <Arduino.h>
#include <stdint.h>

// Адреса на шине I2C (выбираются перемычкой SDO)
#define BMP280_I2C_ADDR_PRIMARY     0x76
#define BMP280_I2C_ADDR_SECONDARY   0x77

// Регистры BMP280
#define BMP280_REG_CALIB            0x88
#define BMP280_REG_CHIP_ID          0xD0
#define BMP280_REG_CTRL_MEAS        0xF4
#define BMP280_REG_CONFIG           0xF5
#define BMP280_REG_DATA             0xF7
#define BMP280_REG_TEMP_MSB         0xFA

// Идентификаторы чипа
#define BMP280_CHIP_ID              0x58
#define BMP280_CHIP_ID_BME280       0x60

// ctrl_meas: osrs_t x2, osrs_p x16, normal mode
#define BMP280_CTRL_MEAS_NORMAL     0x57
// config: standby 0.5 ms, filter x4, SPI 3-wire off
#define BMP280_CONFIG_FILTER_X4     0x08

// Период опроса, мс
#define BMP280_POLL_MS              1000

// ============================================================
// Калибровочные коэффициенты (читаются из 0x88..0xA1)
// ============================================================
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} strBmp280Calib;

#endif // _MODULE_BMP280_TYPES_h
