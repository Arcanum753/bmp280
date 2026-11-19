# module_bmp280

Модуль датчика BMP280 для прошивки [avr-fota](https://github.com/Arcanum753/avr-fota).

## Description

**RU.** Опциональный модуль BMP280 (I2C): температура и атмосферное давление. Публикует
на ресурсную шину `core_state` доступность и значения, показывает их на веб-странице и
в последовательном терминале. Настроек нет: адрес определяется автоматически, параметры
датчика фиксированы.

**EN.** Optional BMP280 module (I2C): temperature and barometric pressure. Publishes
availability and readings to the `core_state` resource bus and exposes them via a web page
and the serial terminal. No settings: the address is auto-detected, sensor configuration is
fixed.

## Features

- I2C addresses `0x76`/`0x77`, auto-detection; `CHIP_ID == 0x58`.
- Bosch integer compensation; pressure converted from Q24.8 to hPa.
- Read-only resources: `bmp280.available`, `bmp280.temperature`, `bmp280.pressure`.
- Web page `bmp280.html` with 1-second refresh (no editing).
- Terminal command `bmp` (`show`, `help`).
- Hot-plug: if the sensor is lost, it is re-detected on the next poll.
- No config file and no `save` function (nothing to persist).

## Resources

| Resource | Type | Description |
|---|---|---|
| `bmp280.available` | BOOL ro | sensor found, calibration valid |
| `bmp280.temperature` | F32 ro | temperature, °C |
| `bmp280.pressure` | F32 ro | pressure, hPa |

## Web and terminal

- `GET /bmp280/info` — state, address, chip id, readings (CVT).
- `GET /bmp280/read` — readings only (polled every second).
- `GET /bmp280/ver` — module version.
- `bmp`, `bmp show`, `bmp help`.

## Macro example

```lua
every("*/15 * * * * *", function()
    print("BMP280", bmp280.available, bmp280.temperature, bmp280.pressure)
end)
```

Ready-to-use cron examples for all three sensors:
`module_macros/web/macros/sensors_15s.lua`, `sensors_30s.lua`, `sensors_1m.lua`,
`sensors_5m.lua`.

## Installation

1. Clone this repository into `src/module_bmp280`.
2. Add `+<module_bmp280/>` to the target env `src_filter`.
3. Add `src/module_bmp280/module_bmp280.ini` to `extra_configs`.
4. Build the env; the module is registered via `[registry]`.

## Tested

2026-11-19, ESP32, env `esp32-i2c-mapper`, firmware `0.117.20261119_1754.2104`:

- Address `0x76`, `CHIP_ID 0x58`.
- Readings: `27.09 °C`, `972.2 hPa`.
- Web pages/endpoints and terminal commands verified; resources present in
  `/state/catalog` and `/macros/resources.json`.
- Fixed during testing: Bosch pressure is Q24.8 (divide by 25600 for hPa).

## Documentation

See `AGENTS.md` for architecture, requirements and limitations.
