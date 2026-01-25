# ESP32-S3-Zero Alarm Clock / Будильник на ESP32-S3-Zero

![Clock](https://github.com/CheshirCa/ESP32-S3-Zero-alarm-clock/blob/main/doc/Clock.jpg)
![Clock from back](https://github.com/CheshirCa/ESP32-S3-Zero-alarm-clock/blob/main/doc/Back(Active_Buzzer).jpg)
![Clock - Info screen](https://github.com/CheshirCa/ESP32-S3-Zero-alarm-clock/blob/main/doc/Info1.jpg)
![Clock - weather](https://github.com/CheshirCa/ESP32-S3-Zero-alarm-clock/blob/main/doc/Weather.jpg)
![Clock - webpage](https://github.com/CheshirCa/ESP32-S3-Zero-alarm-clock/blob/main/doc/WebPage.jpg)
![Clock - Schematic Diagram](https://github.com/CheshirCa/ESP32-S3-Zero-alarm-clock/blob/main/doc/ClockSchematicDiagram.jpg)

# ESP32-S3 Smart Clock / Умные часы на ESP32-S3

[English](#english) | [Русский](#russian)

---

## English

### Features

**Display & Time**
- 2.8" TFT display (240x320, ILI9341)
- Real-time clock with NTP synchronization
- Multiple time zones support with DST
- Weather display (OpenWeatherMap API)

**Alarms & Timers**
- 10 independent alarms with flexible scheduling
  - One-time alarms (specific date)
  - Recurring alarms (days of week)
  - Daily alarms
- Countdown timer with custom text
- Custom melodies for alarms and timer (passive buzzer)
- Auto-stop after 3 minutes

**Smart WiFi Management**
- Two modes: Always ON / Smart WiFi
- Smart WiFi: automatic on/off for sync and web access
- Energy saving: turns off when idle

**Night Mode**
- Automatic brightness reduction during specified hours
- Customizable time range and brightness level

**Sensors**
- AHT20 temperature and humidity sensor
- BMP280 atmospheric pressure sensor
- Real-time sensor data display

**Control**
- Web interface for full configuration
- Serial console commands (EN/RU)
- Physical button (BOOT) for quick actions

**LED Indicator**
- Status indication (WiFi, alarms, timer)
- RGB LED with color coding

### Hardware Requirements

- ESP32-S3 development board
- 2.8" TFT ILI9341 display (SPI)
- AHT20 sensor (I2C, address 0x38)
- BMP280 sensor (I2C, address 0x76)
- Passive or active buzzer
- RGB LED (or separate R/G/B LEDs)
- Push button (connected to BOOT pin)

### Quick Start

1. Install Arduino IDE with ESP32 support
2. Install required libraries (see MANUAL.md)
3. Configure hardware connections in code
4. Upload firmware to ESP32-S3
5. Connect to setup WiFi AP or configure via Serial
6. Access web interface at device IP address

See [MANUAL.md](MANUAL.md) for detailed installation and usage instructions.

---

## Russian

### Функционал

**Дисплей и время**
- TFT дисплей 2.8" (240x320, ILI9341)
- Часы реального времени с синхронизацией NTP
- Поддержка часовых поясов и летнего времени
- Отображение погоды (OpenWeatherMap API)

**Будильники и таймеры**
- 10 независимых будильников с гибкой настройкой
  - Разовые будильники (на конкретную дату)
  - Повторяющиеся будильники (дни недели)
  - Ежедневные будильники
- Таймер обратного отсчета с текстовым названием
- Настраиваемые мелодии для будильника и таймера (пассивный зуммер)
- Автоостановка через 3 минуты

**Умное управление WiFi**
- Два режима: Всегда включен / Умный WiFi
- Умный WiFi: автоматическое включение/выключение для синхронизации и веб-доступа
- Энергосбережение: выключается при отсутствии активности

**Ночной режим**
- Автоматическое снижение яркости в указанные часы
- Настраиваемый диапазон времени и уровень яркости

**Датчики**
- Датчик температуры и влажности AHT20
- Датчик атмосферного давления BMP280
- Отображение данных датчиков в реальном времени

**Управление**
- Веб-интерфейс для полной настройки
- Консольные команды (EN/RU)
- Физическая кнопка (BOOT) для быстрых действий

**Световой индикатор**
- Индикация состояния (WiFi, будильники, таймер)
- RGB светодиод с цветовой кодировкой

### Требования к оборудованию

- Плата разработки ESP32-S3
- TFT дисплей 2.8" ILI9341 (SPI)
- Датчик AHT20 (I2C, адрес 0x38)
- Датчик BMP280 (I2C, адрес 0x76)
- Пассивный или активный зуммер
- RGB светодиод (или отдельные R/G/B светодиоды)
- Кнопка (подключена к пину BOOT)

### Быстрый старт

1. Установите Arduino IDE с поддержкой ESP32
2. Установите необходимые библиотеки (см. MANUAL.md)
3. Настройте подключение оборудования в коде
4. Загрузите прошивку на ESP32-S3
5. Подключитесь к WiFi точке доступа для настройки или настройте через Serial
6. Откройте веб-интерфейс по IP адресу устройства

Подробные инструкции по установке и использованию см. в [MANUAL.md](MANUAL.md).
