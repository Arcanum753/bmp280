# module_bmp280

Модуль датчика BMP280 (температура и атмосферное давление) для прошивки
[avr-fota](https://github.com/Arcanum753/avr-fota).

- I2C, адреса `0x76`/`0x77` (автоопределение), `CHIP_ID = 0x58`.
- Ресурсы шины: `bmp280.available`, `bmp280.temperature` (°C), `bmp280.pressure` (hPa).
- Веб-страница `bmp280.html` — только показ значений.
- Терминал: `bmp`, `bmp show`, `bmp help`.
- Конфига нет; параметры датчика фиксированы.

Подключение: склонировать репозиторий в `src/module_bmp280` и добавить
`+<module_bmp280/>` в `src_filter` нужного env. Подробности — `AGENTS.md`.
