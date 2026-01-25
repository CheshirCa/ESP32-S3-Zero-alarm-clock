#include <U8g2lib.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <time.h>
#include <Wire.h>
#include <Preferences.h>
#include <esp_timer.h>
#include <Adafruit_NeoPixel.h>
#include <driver/ledc.h> 
#include <esp32-hal-ledc.h>
#include <Adafruit_AHTX0.h>
#include <nvs_flash.h>
#include "webpage.h"
#include "setuppage.h"

SemaphoreHandle_t i2cMutex = NULL;

#define BUZZER_ACTIVE 0
#define BUZZER_PASSIVE 1
#define BUZZER_TYPE BUZZER_PASSIVE
#define LOW_LEVEL_TRIGGER 1

#define BOOT_PIN 0
#define BUZZER_PIN 12
#define WS2812_PIN 21
#define NUM_PIXELS 1
#define BUZZER_LEDC_CHANNEL 0
#define BUZZER_LEDC_RESOLUTION 8

#define SDA_PIN 10
#define SCL_PIN 11

#define MAX_ALARMS 10
#define I2C_TIMEOUT_MS 100
#define I2C_MAX_RETRIES 3

// Include webserver module - MUST be after #defines
#include "clockwebserver.h"


U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 6, 5);

const int DISP_W = 128;
const int DISP_H = 64;
const int X_OFF = 0;
const int Y_OFF = 20;
const int YELLOW_ZONE_MAX = 20;
const int BLUE_ZONE_MIN = 21;
const int BLUE_ZONE_MAX = 64;
const int INFO_Y_OFFSET = 10;

String defSSID = "Enter_SSID";
String defPASS = "Enter_Pass";
String defNTP = "pool.ntp.org";
long defGMTOffset = 3 * 3600;
long defDaylightOffset = 0;
String defCity = "Moscow";
const int defBrightness = 255;
String defAlarmMelody = "E5 E P S E5 E P S E5 Q P Q E5 E P S E5 E P S E5 Q P Q E5 E P S G5 E P S C5 E P S D5 E P S E5 H P H";
String defTimerMelody = "G6 E A6 E G6 E F6 E P W";

String wifiSSID;
String wifiPASS;
String ntpServer;
long gmtOffset_sec;
long daylightOffset_sec;
String cityName;
int displayBrightness;
String alarmMelody;
String timerMelody;

// Theme and Language settings
String currentTheme = "light";
String currentLanguage = "en";

// Translations for Serial output
struct Translation {
  const char* en;
  const char* ru;
};

Translation translations[] = {
  {"Initializing...", "Инициализация..."},
  {"NVS initialized OK", "NVS инициализирован"},
  {"WiFi SSID:", "WiFi SSID:"},
  {"WARNING: Using default WiFi settings!", "ВНИМАНИЕ: Используются настройки WiFi по умолчанию!"},
  {"Configure WiFi via Serial commands or web interface", "Настройте WiFi через Serial команды или веб-интерфейс"},
  {"[Smart WiFi] Initial sync complete, WiFi OFF", "[Умный WiFi] Начальная синхронизация завершена, WiFi ВЫКЛ"},
  {"Failed to create I2C mutex!", "Ошибка создания I2C mutex!"},
  {"Initializing sensors...", "Инициализация датчиков..."},
  {"AHT20: OK (0x38 on Wire1)", "AHT20: OK (0x38 на Wire1)"},
  {"BMP280: OK (direct access)", "BMP280: OK (прямой доступ)"},
  {"BMP280: Not found", "BMP280: Не найден"},
  {"Failed to acquire I2C mutex after retries", "Не удалось получить I2C mutex после повторных попыток"},
  {"Setup complete!", "Настройка завершена!"},
  {"Web server: RUNNING", "Веб-сервер: РАБОТАЕТ"},
  {"Web interface: http://", "Веб-интерфейс: http://"},
  {"WiFi Signal:", "WiFi Сигнал:"},
  {"WiFi: DISCONNECTED", "WiFi: ОТКЛЮЧЕН"},
  {"Will auto-connect for sync or web access", "Автоподключение для синхронизации или веб-доступа"},
  {"Available commands:", "Доступные команды:"},
  {"WEATHER", "ПОГОДА"},
  {"No WiFi", "Нет WiFi"},
  {"No data", "Нет данных"},
  {"Connection failed", "Ошибка подключения"},
  {"=== CLOCK COMMANDS ===", "=== КОМАНДЫ ЧАСОВ ==="},
  {"TIME YYYY-MM-DD HH:MM:SS - Set time manually", "TIME ГГГГ-ММ-ДД ЧЧ:ММ:СС - Установить время вручную"},
  {"NTP <server> - Set NTP server", "NTP <сервер> - Установить NTP сервер"},
  {"TZ <+/-hours> - Set timezone offset", "TZ <+/-часы> - Установить часовой пояс"},
  {"DST <+/-hours> - Set daylight saving offset", "DST <+/-часы> - Установить летнее время"},
  {"CITY <n> - Set city for weather", "CITY <название> - Установить город для погоды"},
  {"BRIGHTNESS <0-255> - Set display brightness", "BRIGHTNESS <0-255> - Установить яркость дисплея"},
  {"SAVE - Save settings to NVS", "SAVE - Сохранить настройки в NVS"},
  {"RESTORE - Restore settings from NVS", "RESTORE - Восстановить настройки из NVS"},
  {"ERASE - Erase all NVS data", "ERASE - Стереть все данные NVS"},
  {"STATUS - Show system status", "STATUS - Показать статус системы"},
  {"SYNC - Force NTP time sync", "SYNC - Принудительная синхронизация NTP"},
  {"WEATHER - Update weather data", "WEATHER - Обновить данные погоды"},
  {"REBOOT - Restart device", "REBOOT - Перезагрузить устройство"},
  {"LANGUAGE <EN|RU> - Switch interface language", "LANGUAGE <EN|RU> - Переключить язык интерфейса"},
  {"=== ALARM COMMANDS ===", "=== КОМАНДЫ БУДИЛЬНИКА ==="},
  {"ALARM LIST - List all alarms", "ALARM LIST - Список всех будильников"},
  {"ALARM SET <index> [YYYY-MM-DD|1234567] HH:MM [TEXT] [R] [S]", "ALARM SET <индекс> [ГГГГ-ММ-ДД|1234567] ЧЧ:ММ [ТЕКСТ] [R] [S]"},
  {"ALARM CLEAR <index> - Clear alarm", "ALARM CLEAR <индекс> - Очистить будильник"},
  {"ALARM CLEARALL - Clear all alarms", "ALARM CLEARALL - Очистить все будильники"},
  {"ALARM TOGGLE <index> - Toggle alarm on/off", "ALARM TOGGLE <индекс> - Вкл/выкл будильник"},
  {"=== TIMER COMMANDS ===", "=== КОМАНДЫ ТАЙМЕРА ==="},
  {"TIMER HH:MM:SS [TEXT] - Start timer", "TIMER ЧЧ:ММ:СС [ТЕКСТ] - Запустить таймер"},
  {"TIMER MM:SS [TEXT] - Start timer", "TIMER ММ:СС [ТЕКСТ] - Запустить таймер"},
  {"TIMER SS [TEXT] - Start timer", "TIMER СС [ТЕКСТ] - Запустить таймер"},
  {"TIMER CLEAR - Clear timer", "TIMER CLEAR - Очистить таймер"},
  {"=== WiFi MODE COMMANDS ===", "=== КОМАНДЫ РЕЖИМА WiFi ==="},
  {"WIFI SET <ssid> <password> - Set WiFi credentials", "WIFI SET <ssid> <пароль> - Установить данные WiFi"},
  {"WIFI ALWAYS - Set WiFi mode to ALWAYS ON", "WIFI ALWAYS - Режим WiFi: ВСЕГДА ВКЛ"},
  {"WIFI SMART - Set WiFi mode to SMART WiFi", "WIFI SMART - Режим WiFi: УМНЫЙ WiFi"},
  {"WIFI ON - Manually enable WiFi for 10 min (Smart mode)", "WIFI ON - Вручную включить WiFi на 10 мин (Умный режим)"},
  {"WIFI OFF - Turn WiFi off", "WIFI OFF - Выключить WiFi"},
  {"WIFI STATUS - Show WiFi mode and status", "WIFI STATUS - Показать режим и статус WiFi"},
  {"=== NIGHT MODE COMMANDS ===", "=== КОМАНДЫ НОЧНОГО РЕЖИМА ==="},
  {"NIGHT ON <HH:MM-HH:MM> <brightness> - Enable night mode", "NIGHT ON <ЧЧ:ММ-ЧЧ:ММ> <яркость> - Включить ночной режим"},
  {"NIGHT OFF - Disable night mode", "NIGHT OFF - Выключить ночной режим"},
  {"NIGHT STATUS - Show night mode settings", "NIGHT STATUS - Показать настройки ночного режима"},
  {"WiFi settings updated:", "Настройки WiFi обновлены:"},
  {"Password:", "Пароль:"},
  {"Connecting to WiFi...", "Подключение к WiFi..."},
  {"WiFi connected successfully!", "WiFi подключен успешно!"},
  {"IP Address:", "IP адрес:"},
  {"Use SAVE command to store settings to NVS", "Используйте команду SAVE для сохранения в NVS"},
  {"WiFi connection failed!", "Ошибка подключения WiFi!"},
  {"Check SSID and password, then try again", "Проверьте SSID и пароль, затем попробуйте снова"},
  {"Disabling WiFi...", "Отключение WiFi..."},
  {"WiFi disabled", "WiFi отключен"},
  {"NTP server set to:", "NTP сервер установлен:"},
  {"Syncing time with NTP server...", "Синхронизация времени с NTP сервером..."},
  {"Time synchronized successfully!", "Время синхронизировано успешно!"},
  {"Current time:", "Текущее время:"},
  {"Time sync failed!", "Ошибка синхронизации времени!"},
  {"WiFi not connected - time sync skipped", "WiFi не подключен - синхронизация пропущена"},
  {"NTP server will be used after WiFi connects", "NTP сервер будет использован после подключения WiFi"},
  {"Updating weather...", "Обновление погоды..."},
  {"WiFi not connected - weather update skipped", "WiFi не подключен - обновление погоды пропущено"},
  // Status menu items
  {"  status     - Show system status", "  status     - Показать статус системы"},
  {"  alarm list - Show all alarms", "  alarm list - Показать все будильники"},
  {"  wifi ...   - WiFi management", "  wifi ...   - Управление WiFi"},
  {"  help       - Show all commands", "  help       - Показать все команды"},
  // Status messages
  {"[Night Mode] DISABLED", "[Ночной режим] ВЫКЛЮЧЕН"},
  {"[Night Mode] ENABLED", "[Ночной режим] ВКЛЮЧЕН"},
  {"[Night Mode] Screen brightness restored", "[Ночной режим] Яркость экрана восстановлена"},
  {"[Night Mode] Time:", "[Ночной режим] Время:"},
  {"[Night Mode] Brightness:", "[Ночной режим] Яркость:"},
  {"[Night Mode] Currently Active:", "[Ночной режим] Сейчас активен:"},
  {"[Night Mode] Usage: NIGHT ON HH:MM-HH:MM [brightness]", "[Ночной режим] Использование: NIGHT ON ЧЧ:ММ-ЧЧ:ММ [яркость]"},
  {"[Night Mode] Example: NIGHT ON 23:00-07:00 10", "[Ночной режим] Пример: NIGHT ON 23:00-07:00 10"},
  {"[Night Mode] Available commands:", "[Ночной режим] Доступные команды:"},
  {"[WiFi] Mode:", "[WiFi] Режим:"},
  {"[WiFi] Status:", "[WiFi] Статус:"},
  {"WiFi Status:", "Статус WiFi:"},
  {"Night Mode:", "Ночной режим:"},
  {"WiFi Mode:", "Режим WiFi:"},
  {"YES", "ДА"},
  {"NO", "НЕТ"},
  {"CONNECTED", "ПОДКЛЮЧЕН"},
  {"DISCONNECTED", "ОТКЛЮЧЕН"},
  {"ALWAYS ON", "ВСЕГДА ВКЛ"},
  {"SMART", "УМНЫЙ"},
  {"ENABLED", "ВКЛЮЧЕН"},
  {"DISABLED", "ВЫКЛЮЧЕН"},
  // Setup mode
  {"[SETUP MODE] Starting WiFi AP...", "[РЕЖИМ НАСТРОЙКИ] Запуск WiFi точки доступа..."},
  {"[SETUP MODE] AP Started", "[РЕЖИМ НАСТРОЙКИ] Точка доступа запущена"},
  {"[SETUP MODE] SSID:", "[РЕЖИМ НАСТРОЙКИ] SSID:"},
  {"[SETUP MODE] Connect to configure", "[РЕЖИМ НАСТРОЙКИ] Подключитесь для настройки"},
  {"[SETUP MODE] Configuration saved, rebooting...", "[РЕЖИМ НАСТРОЙКИ] Настройки сохранены, перезагрузка..."},
  {"[SETUP MODE] Exiting...", "[РЕЖИМ НАСТРОЙКИ] Выход..."},
  // Button messages
  {"[Button] ===== LONG PRESS DETECTED =====", "[Кнопка] ===== ДОЛГОЕ НАЖАТИЕ ОБНАРУЖЕНО ====="},
  {"[Button] WiFi manually enabled for 10 minutes", "[Кнопка] WiFi вручную включен на 10 минут"},
  {"[Button] Current WiFi status:", "[Кнопка] Текущий статус WiFi:"},
  {"[Button] IP Address:", "[Кнопка] IP адрес:"},
  {"[Button] WiFi will turn ON in next loop cycle", "[Кнопка] WiFi включится в следующем цикле"},
  {"[Button] ==============================", "[Кнопка] =============================="},
  {"[Button] Long press detected, but WiFi mode is ALWAYS ON", "[Кнопка] Долгое нажатие обнаружено, но WiFi в режиме ВСЕГДА ВКЛ"},
  {"[Button] Use 'WIFI SMART' command to enable Smart WiFi mode", "[Кнопка] Используйте команду 'WIFI SMART' для включения Умного WiFi"},
  {"[Button] ===== SETUP MODE ACTIVATED =====", "[Кнопка] ===== РЕЖИМ НАСТРОЙКИ АКТИВИРОВАН ====="},
  {"[Button] Hold for 10 seconds to enter setup mode", "[Кнопка] Удерживайте 10 секунд для входа в режим настройки"},
  {"[Button] Release now for WiFi enable (3 sec)", "[Кнопка] Отпустите сейчас для включения WiFi (3 сек)"},
  {"CONNECTED", "ПОДКЛЮЧЕН"},
  {"NO SHIELD", "НЕТ МОДУЛЯ"},
  {"IDLE", "ОЖИДАНИЕ"},
  {"NO SSID AVAILABLE", "SSID НЕДОСТУПЕН"},
  {"SCAN COMPLETED", "СКАНИРОВАНИЕ ЗАВЕРШЕНО"},
  {"CONNECT FAILED", "ОШИБКА ПОДКЛЮЧЕНИЯ"},
  {"CONNECTION LOST", "СОЕДИНЕНИЕ ПОТЕРЯНО"},
  {"DISCONNECTED", "ОТКЛЮЧЕН"},
  {"UNKNOWN", "НЕИЗВЕСТНО"},
  // Language command
  {"Language changed to:", "Язык изменен на:"},
  {"English", "Английский"},
  {"Русский", "Русский"},
  {"Settings saved. Restart to see changes in startup messages.", "Настройки сохранены. Перезагрузитесь чтобы увидеть изменения в стартовых сообщениях."},
  // STATUS command
  {"=== SYSTEM STATUS ===", "=== СТАТУС СИСТЕМЫ ==="},
  {"WiFi SSID:", "WiFi SSID:"},
  {"Signal Strength:", "Сила сигнала:"},
  {"NTP Server:", "NTP Сервер:"},
  {"Time Sync:", "Синхронизация времени:"},
  {"Timezone:", "Часовой пояс:"},
  {"DST Offset:", "Летнее время:"},
  {"Weather City:", "Город погоды:"},
  {"Display Brightness:", "Яркость дисплея:"},
  {"Buzzer Type: PASSIVE", "Тип зуммера: ПАССИВНЫЙ"},
  {"Buzzer Type: ACTIVE", "Тип зуммера: АКТИВНЫЙ"},
  {"Alarm Melody:", "Мелодия будильника:"},
  {"Timer Melody:", "Мелодия таймера:"},
  {"Active:", "Активен:"},
  {"Time:", "Время:"},
  {"Brightness:", "Яркость:"},
  {"Auto-off timeout:", "Таймаут авто-выкл:"},
  {"NTP Sync interval:", "Интервал синхронизации NTP:"},
  {"Weather Sync interval:", "Интервал обновления погоды:"},
  {"Manual mode:", "Ручной режим:"},
  {"Time until auto-off:", "Время до авто-выкл:"},
  {"WiFi is OFF (will wake for sync or web access)", "WiFi ВЫКЛ (включится для синхронизации или веб-доступа)"},
  {"Next NTP sync in:", "Следующая синхронизация NTP через:"},
  {"Next weather update in:", "Следующее обновление погоды через:"},
  {"sec remaining", "сек осталось"},
  {"sec", "сек"},
  {"min", "мин"},
  {"hours", "час"},
  {"dBm", "дБм"},
  {"web activity", "веб активность"},
  {"GMT", "GMT"},
  // Additional status messages
  {"Current Date:", "Текущая дата:"},
  {"Temperature:", "Температура:"},
  {"Humidity:", "Влажность:"},
  {"Pressure:", "Давление:"},
  {"Free RAM:", "Свободная RAM:"},
  {"KB", "КБ"},
  {"=== ALARMS ===", "=== БУДИЛЬНИКИ ==="},
  // Melody commands
  {"=== MELODY COMMANDS (Passive Buzzer) ===", "=== КОМАНДЫ МЕЛОДИЙ (Пассивный зуммер) ==="},
  {"MELODY ALARM <notes> - Set alarm melody", "MELODY ALARM <ноты> - Установить мелодию будильника"},
  {"MELODY TIMER <notes> - Set timer melody", "MELODY TIMER <ноты> - Установить мелодию таймера"},
  {"MELODY TEST <notes> - Test melody", "MELODY TEST <ноты> - Проверить мелодию"},
  {"MELODY SAVE - Save melodies to NVS", "MELODY SAVE - Сохранить мелодии в NVS"},
  // Status labels
  {"WiFi SSID:", "WiFi SSID:"},
  {"IP Address:", "IP адрес:"},
  {"Signal Strength:", "Сила сигнала:"},
  {"Alarm Melody:", "Мелодия будильника:"},
  {"Timer Melody:", "Мелодия таймера:"},
  {"Current Time:", "Текущее время:"},
  // WiFi status values
  {"ALWAYS ON", "ВСЕГДА ВКЛ"},
  {"CONNECTED", "ПОДКЛЮЧЕН"},
  // Alarm list values
  {"ON", "ВКЛ"},
  {"OFF", "ВЫКЛ"},
  {"R", "П"},  // Repeat = Повтор
  // Save messages
  {"Saving settings to NVS...", "Сохранение настроек в NVS..."},
  {"  - WiFi config saved", "  - Конфигурация WiFi сохранена"},
  {"  - Display settings saved", "  - Настройки дисплея сохранены"},
  {"  - Melody settings saved", "  - Настройки мелодий сохранены"},
  {"All settings saved successfully!", "Все настройки успешно сохранены!"},
  // Restore messages
  {"Restoring settings from NVS...", "Восстановление настроек из NVS..."},
  {"  - WiFi config loaded", "  - Конфигурация WiFi загружена"},
  {"  - Display settings loaded", "  - Настройки дисплея загружены"},
  {"  - Melody settings loaded", "  - Настройки мелодий загружены"},
  {"Connecting to WiFi (", "Подключение к WiFi ("},
  // Sync command
  {"Synchronizing time with NTP server...", "Синхронизация времени с NTP сервером..."},
  {"Current time:", "Текущее время:"},
  {"Current date:", "Текущая дата:"},
  {"Time synchronization failed!", "Ошибка синхронизации времени!"},
  {"Check NTP server and network connection", "Проверьте NTP сервер и сетевое подключение"},
  {"WiFi not connected!", "WiFi не подключен!"},
  {"Connect to WiFi first using: WIFI <ssid> <password>", "Сначала подключитесь к WiFi: WIFI <ssid> <пароль>"},
  // Smart WiFi messages
  {"[Smart WiFi] ===== TURNING WiFi ON =====", "[Умный WiFi] ===== ВКЛЮЧЕНИЕ WiFi ====="},
  {"[Smart WiFi] Reason:", "[Умный WiFi] Причина:"},
  {"Manual enable (button/command)", "Ручное включение (кнопка/команда)"},
  {"Recent web activity", "Недавняя веб активность"},
  {"NTP sync needed", "Требуется синхронизация NTP"},
  {"Weather update needed", "Требуется обновление погоды"},
  {"[Smart WiFi] Connecting...", "[Умный WiFi] Подключение..."},
  {"[Smart WiFi] ============================", "[Умный WiFi] ============================"},
  {"[Smart WiFi] ===== TURNING WiFi OFF =====", "[Умный WiFi] ===== ВЫКЛЮЧЕНИЕ WiFi ====="},
  {"[Smart WiFi] No activity, powering down", "[Умный WiFi] Нет активности, отключение"},
  {"[Smart WiFi] WiFi powered OFF", "[Умный WiFi] WiFi ВЫКЛЮЧЕН"},
  {"[Smart WiFi] =============================", "[Умный WiFi] ============================="},
  {"[Smart WiFi] ===== CONNECTION SUCCESS =====", "[Умный WiFi] ===== ПОДКЛЮЧЕНИЕ УСПЕШНО ====="},
  {"[Smart WiFi] IP Address:", "[Умный WiFi] IP адрес:"},
  {"[Smart WiFi] Signal:", "[Умный WiFi] Сигнал:"},
  {"[Smart WiFi] ===========================", "[Умный WiFi] ==========================="},
  {"[Smart WiFi] ===== CONNECTION TIMEOUT =====", "[Умный WiFi] ===== ТАЙМАУТ ПОДКЛЮЧЕНИЯ ====="},
  {"[Smart WiFi] Failed to connect in 20 sec", "[Умный WiFi] Не удалось подключиться за 20 сек"},
  {"[Smart WiFi] Will retry on next cycle", "[Умный WiFi] Повторная попытка в следующем цикле"},
  {"[Smart WiFi] ===============================", "[Умный WiFi] ==============================="},
  {"[Smart WiFi] NTP sync", "[Умный WiFi] Синхронизация NTP"},
  {"[Smart WiFi] Weather update scheduled", "[Умный WiFi] Обновление погоды запланировано"},
  // Alarm messages
  {"[Alarm] Cleared alarm", "[Будильник] Очищен будильник"},
  {"[Alarm] All alarms cleared", "[Будильник] Все будильники очищены"},
  {"[Alarm] Alarm", "[Будильник] Будильник"},
  {"is now", "теперь"},
  {"ON", "ВКЛ"},
  {"OFF", "ВЫКЛ"},
  {"[Alarm] Melody set for alarm", "[Будильник] Мелодия установлена для будильника"},
  {"[Alarm] Invalid index", "[Будильник] Неверный индекс"},
  {"[Alarm] Set alarm", "[Будильник] Установлен будильник"},
  {"for", "на"},
  {"[Alarm] Use: ALARM SET <index> ... or ALARM LIST/CLEAR/TOGGLE", "[Будильник] Используйте: ALARM SET <индекс> ... или ALARM LIST/CLEAR/TOGGLE"},
  {"[Alarm] Auto-stop after 3 minutes", "[Будильник] Автоостановка через 3 минуты"},
  // Timer messages
  {"[Timer] Set for", "[Таймер] Установлен на"},
  {"[Timer] Auto-stop after 3 minutes", "[Таймер] Автоостановка через 3 минуты"},
  {"Timer:", "Таймер:"},
  {"remaining", "осталось"},
  {"Alarms:", "Будильники:"},
  {"active (use ALARM LIST for details)", "активны (используйте ALARM LIST для деталей)"},
  // Button messages
  {"[Button] Alarm/Timer stopped", "[Кнопка] Будильник/Таймер остановлен"},
  {"[Button] Screen temporarily enabled for 1 minute", "[Кнопка] Экран временно включен на 1 минуту"},
  // Screen messages
  {"[Screen] Temporary enable timeout - screen off", "[Экран] Время временного включения истекло - экран выключен"}
};

const int translationsCount = sizeof(translations) / sizeof(Translation);

String TR(const char* text) {
  if (currentLanguage == "ru") {
    for (int i = 0; i < translationsCount; i++) {
      if (strcmp(translations[i].en, text) == 0) {
        return String(translations[i].ru);
      }
    }
  }
  return String(text);
}

void SerialPrintTR(const char* text) {
  Serial.println(TR(text));
}

// Night mode settings
bool nightModeEnabled = false;
int nightStartHour = 23;
int nightStartMinute = 0;
int nightEndHour = 7;
int nightEndMinute = 0;
int nightBrightness = 10;
bool nightModeActive = false;

// Smart WiFi settings
// WiFiMode defined in ClockWebServer.h
WiFiMode wifiMode = WIFI_ALWAYS_ON;  // Default to always on for compatibility
unsigned long lastWebRequest = 0;
unsigned long lastNtpSync = 0;
unsigned long lastWeatherSync = 0;
unsigned long wifiAutoOffTimeout = 10 * 60 * 1000; // 10 minutes
unsigned long ntpSyncInterval = 60 * 60 * 1000;    // 60 minutes
unsigned long weatherSyncInterval = 30 * 60 * 1000; // 30 minutes
bool wifiManuallyEnabled = false;
unsigned long wifiManualTimeout = 0;

// BMP280 переменные (прямой доступ)
int32_t _t_fine;
uint16_t _dig_T1;
int16_t _dig_T2, _dig_T3;
uint16_t _dig_P1;
int16_t _dig_P2, _dig_P3, _dig_P4, _dig_P5, _dig_P6, _dig_P7, _dig_P8, _dig_P9;

WebServer server(80);
DNSServer dnsServer;

// Setup Mode (Captive Portal)
bool setupMode = false;
bool setupModeActive = false;
unsigned long bootButtonPressStart = 0;
const unsigned long BOOT_LONG_PRESS = 10000; // 10 seconds
bool bootButtonWasPressed = false;
unsigned long setupLedBlink = 0;
bool setupLedState = false;
const unsigned long SETUP_LED_INTERVAL = 200; // Fast blink
const char* SETUP_AP_SSID = "ESP32-Clock-Setup";
const char* SETUP_AP_PASS = "";
String setupTempLanguage = "en"; // Temporary language for setup page

Preferences prefs;
const char* PREF_NS = "clockcfg";
const char* ALARM_PREF_NS = "alarms";
const char* DISPLAY_PREF_NS = "display";
const char* MELODY_PREF_NS = "melodies";
const char* NIGHT_PREF_NS = "nightmode";
const char* WIFI_PREF_NS = "wifimode";
const char* THEME_PREF_NS = "theme";

struct tm timeinfo;
bool timeValid = false;
bool buzzerActive = false;  // Track if tone is currently playing

enum ScreenMode {
  SCREEN_CLOCK,
  SCREEN_INFO1,
  SCREEN_INFO2,
  SCREEN_INFO3,
  SCREEN_INFO4,
  SCREEN_ALARM,
  SCREEN_TIMER
};

ScreenMode currentScreen = SCREEN_CLOCK;
int infoScreenPage = 1;
const unsigned long INFO_TIMEOUT = 10000;
unsigned long infoStartTime = 0;
bool colonVisible = true;
unsigned long lastBlink = 0;

const unsigned long BTN_DEBOUNCE = 300;
unsigned long lastBtnTime = 0;

unsigned long lastScreenUpdate = 0;
const unsigned long SCREEN_INFO_INTERVAL = 100;
const unsigned long SCREEN_ALARM_INTERVAL = 20;
const unsigned long SCREEN_CLOCK_INTERVAL = 50;


bool readingSensors = false;
bool updatingWeather = false;
bool needWeatherUpdate = false;

#if BUZZER_TYPE == BUZZER_PASSIVE
const int noteFrequencies[12] = {
  262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494
};

// Note defined in ClockWebServer.h

Note alarmNotes[50];
int alarmNoteCount = 0;
Note timerNotes[50];
int timerNoteCount = 0;
Note customAlarmNotes[50];  // Global array for custom alarm melodies
int currentNoteIndex = 0;
unsigned long noteStartTime = 0;
bool melodyPlaying = false;
Note* currentMelody = nullptr;
int currentMelodyLength = 0;
unsigned long melodyPauseStart = 0;
bool melodyInPause = false;
bool alarmMelodyStarted = false;  // Track if alarm melody already started
bool timerMelodyStarted = false;  // Track if timer melody already started
#endif

Adafruit_NeoPixel pixel(NUM_PIXELS, WS2812_PIN, NEO_GRB + NEO_KHZ800);
const uint32_t COLOR_OFF = pixel.Color(0, 0, 0);
const uint32_t COLOR_ALARM = pixel.Color(0, 0, 255);
const uint32_t COLOR_TIMER = pixel.Color(0, 255, 0);
const uint32_t COLOR_ALARM_TRIG = pixel.Color(255, 0, 0);
const uint32_t COLOR_TIMER_TRIG = pixel.Color(255, 255, 0);
const uint32_t COLOR_BOTH = pixel.Color(128, 0, 128);

char hhStr[3] = "--";
char mmStr[3] = "--";
char dateStr[11] = "00.00.0000";
char weekdayStr[15] = "---";

String serialInput = "";
bool promptShown = false;
const int HISTORY_SIZE = 10;
String commandHistory[HISTORY_SIZE];
int historyIndex = 0;
int historyCount = 0;
int historyBrowseIndex = -1;
String tempInput = "";

// Alarm defined in ClockWebServer.h

#include "ClockWebServer.h"


Alarm alarms[MAX_ALARMS];
int triggeredAlarmIndex = -1;

// Alarm state tracking
static int lastAlarmMinute = -1;
static bool alarmFiredThisMinute = false;

bool timerActive = false;
uint64_t timerStartUs = 0;
uint64_t timerDurationUs = 0;
char timerText[31] = "";

bool timerTriggered = false;
bool alarmTriggered = false;
unsigned long alarmTriggerTime = 0;
unsigned long timerTriggerTime = 0;
const unsigned long ALARM_AUTO_STOP_TIME = 180000; // 3 минуты в миллисекундах

// Temporary screen enable
bool screenTempEnabled = false;
unsigned long screenTempTimeout = 0;

const char* weekdaysRU[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

String weatherData = "";
unsigned long lastWeatherUpdate = 0;
const unsigned long WEATHER_UPDATE_INTERVAL = 1800000;

struct TextLine {
  String text;
  int width;
};

const uint8_t* fontList[] = {
  u8g2_font_inr38_t_cyrillic,
  u8g2_font_inr33_t_cyrillic,
  u8g2_font_inr30_t_cyrillic,
  u8g2_font_inr27_t_cyrillic,
  u8g2_font_inr24_t_cyrillic,
  u8g2_font_10x20_t_cyrillic,
  u8g2_font_unifont_t_cyrillic,
  u8g2_font_8x13_t_cyrillic,
  u8g2_font_6x13_t_cyrillic,
  u8g2_font_6x12_t_cyrillic,
  u8g2_font_5x8_t_cyrillic,
  u8g2_font_4x6_t_cyrillic
};
const int fontListSize = sizeof(fontList) / sizeof(fontList[0]);

Adafruit_AHTX0 aht;
bool ahtFound = false;
bool bmpFound = false;
sensors_event_t humidity, temp;
float temperature = 0.0;
float humidityVal = 0.0;
float pressure = 0.0;
unsigned long lastSensorRead = 0;
const unsigned long SENSOR_READ_INTERVAL = 3000;

void loadConfigFromNVS();
void saveConfigToNVS();
void loadAlarmFromNVS(int index);
void saveAlarmToNVS(int index);
void clearAlarmFromNVS(int index);
void loadAllAlarmsFromNVS();
void saveAllAlarmsToNVS();
void loadDisplaySettings();
void saveDisplaySettings();
void loadMelodySettings();
void saveMelodySettings();
void loadNightModeSettings();
void saveNightModeSettings();
void loadWiFiModeSettings();
void saveWiFiModeSettings();
void loadThemeLanguageSettings();
void saveThemeLanguageSettings();
void checkNightMode();
void eraseNVS();
void updateClockStrings();
void drawClock();
void drawAlarmOrTimer(const char* txt);
void drawInfoScreen1();
void drawInfoScreen2();
void drawInfoScreen3();
void drawInfoScreen4();
void showSplash();
// Setup mode functions
void startSetupMode();
void stopSetupMode();
void handleSetupModeLED();
void setupModeHandlers();
void checkFirstBoot();
void setDisplayBrightness(int brightness);
void connectWiFi();
void syncTime();
void setManualTime(String s);
void updateWeather();
String getSimpleWeather();
void handleButton();
void handleAutoReturn();
void handleSerial();
bool checkAlarmMatch();
void updateLEDIndicator();
void splitWords(String text, String* words, int &wordCount, int maxWords);
bool tryFitText(const uint8_t* font, String text, int areaWidth, int areaHeight, 
                TextLine* lines, int &lineCount, int maxLines);
#if BUZZER_TYPE == BUZZER_PASSIVE
void parseMelody(String melodyStr, Note* notes, int &noteCount, int maxNotes);
void playMelody(Note* notes, int noteCount);
void updateMelodyPlayback();
void stopMelody();
void testMelody(String melodyStr);
void buzzerTone(int pin, int frequency);
void buzzerNoTone(int pin);
void playSimpleBeep();
#endif
void readSensorData();

#if BUZZER_TYPE == BUZZER_PASSIVE
void buzzerTone(int pin, int frequency) {
  #if LOW_LEVEL_TRIGGER
    if (frequency == 0) {
      buzzerNoTone(pin);
      return;
    }
    tone(pin, frequency);
    // НЕ изменяем buzzerActive здесь - это глобальный флаг состояния будильника/таймера
  #else
    tone(pin, frequency);
    // НЕ изменяем buzzerActive здесь - это глобальный флаг состояния будильника/таймера
  #endif
}

void buzzerNoTone(int pin) {
  #if LOW_LEVEL_TRIGGER
    noTone(pin);
    delayMicroseconds(100);
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
  #else
    noTone(pin);
  #endif
  
  // НЕ изменяем buzzerActive здесь - это глобальный флаг состояния будильника/таймера
}

void playSimpleBeep() {
  for (int i = 0; i < 3; i++) {
    buzzerTone(BUZZER_PIN, 1000);
    unsigned long start = millis();
    while (millis() - start < 200) { /* wait */ }
    buzzerNoTone(BUZZER_PIN);
    start = millis();
    while (millis() - start < 100) { /* wait */ }
  }
}
#endif

// ==================== BMP280 DIRECT FUNCTIONS ====================

bool BMP280_begin() {
  // Try multiple times with delays - sensor may need time to stabilize
  for (int attempt = 0; attempt < 3; attempt++) {
    if (attempt > 0) {
      delay(50);  // Wait before retry
    }
    
    Wire1.beginTransmission(0x77);
    if (Wire1.endTransmission() != 0) {
      continue;  // Try again
    }
    
    uint8_t chip_id = readRegisterBMP280(0xD0);
    if (chip_id != 0x58) { // BMP280 chip id
      continue;  // Try again
    }
    
    // Successfully detected - read trimming parameters
    _dig_T1 = readRegisterBMP280(0x88) | (readRegisterBMP280(0x89) << 8);
    _dig_T2 = readRegisterBMP280(0x8A) | (readRegisterBMP280(0x8B) << 8);
    _dig_T3 = readRegisterBMP280(0x8C) | (readRegisterBMP280(0x8D) << 8);
    
    _dig_P1 = readRegisterBMP280(0x8E) | (readRegisterBMP280(0x8F) << 8);
    _dig_P2 = readRegisterBMP280(0x90) | (readRegisterBMP280(0x91) << 8);
    _dig_P3 = readRegisterBMP280(0x92) | (readRegisterBMP280(0x93) << 8);
    _dig_P4 = readRegisterBMP280(0x94) | (readRegisterBMP280(0x95) << 8);
    _dig_P5 = readRegisterBMP280(0x96) | (readRegisterBMP280(0x97) << 8);
    _dig_P6 = readRegisterBMP280(0x98) | (readRegisterBMP280(0x99) << 8);
    _dig_P7 = readRegisterBMP280(0x9A) | (readRegisterBMP280(0x9B) << 8);
    _dig_P8 = readRegisterBMP280(0x9C) | (readRegisterBMP280(0x9D) << 8);
    _dig_P9 = readRegisterBMP280(0x9E) | (readRegisterBMP280(0x9F) << 8);
    
    // Configure BMP280
    writeRegisterBMP280(0xF4, 0x2F); // ctrl_meas: temp oversampling x2, pressure oversampling x16, normal mode
    writeRegisterBMP280(0xF5, 0x14); // config: standby 500ms, filter x16
    
    return true;
  }
  
  return false;  // All attempts failed
}

uint8_t readRegisterBMP280(uint8_t reg) {
  Wire1.beginTransmission(0x77);
  Wire1.write(reg);
  Wire1.endTransmission();
  
  Wire1.requestFrom(0x77, 1);
  return Wire1.read();
}

void writeRegisterBMP280(uint8_t reg, uint8_t value) {
  Wire1.beginTransmission(0x77);
  Wire1.write(reg);
  Wire1.write(value);
  Wire1.endTransmission();
}

float readTemperatureBMP280() {
  Wire1.beginTransmission(0x77);
  Wire1.write(0xFA);
  Wire1.endTransmission();
  
  Wire1.requestFrom(0x77, 3);
  uint8_t msb = Wire1.read();
  uint8_t lsb = Wire1.read();
  uint8_t xlsb = Wire1.read();
  
  int32_t adc_T = ((int32_t)msb << 12) | ((int32_t)lsb << 4) | ((xlsb >> 4) & 0x0F);
  
  // Calculate temperature
  int32_t var1, var2;
  
  var1 = ((((adc_T >> 3) - ((int32_t)_dig_T1 << 1))) * ((int32_t)_dig_T2)) >> 11;
  var2 = (((((adc_T >> 4) - ((int32_t)_dig_T1)) * ((adc_T >> 4) - ((int32_t)_dig_T1))) >> 12) *
          ((int32_t)_dig_T3)) >> 14;
  
  _t_fine = var1 + var2;
  float T = (_t_fine * 5 + 128) >> 8;
  return T / 100.0;
}

float readPressureBMP280() {
  Wire1.beginTransmission(0x77);
  Wire1.write(0xF7);
  Wire1.endTransmission();
  
  Wire1.requestFrom(0x77, 3);
  uint8_t msb = Wire1.read();
  uint8_t lsb = Wire1.read();
  uint8_t xlsb = Wire1.read();
  
  int32_t adc_P = ((int32_t)msb << 12) | ((int32_t)lsb << 4) | ((xlsb >> 4) & 0x0F);
  
  // Calculate pressure
  int64_t var1, var2, p;
  
  var1 = ((int64_t)_t_fine) - 128000;
  var2 = var1 * var1 * (int64_t)_dig_P6;
  var2 = var2 + ((var1 * (int64_t)_dig_P5) << 17);
  var2 = var2 + (((int64_t)_dig_P4) << 35);
  var1 = ((var1 * var1 * (int64_t)_dig_P3) >> 8) + ((var1 * (int64_t)_dig_P2) << 12);
  var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)_dig_P1) >> 33;
  
  if (var1 == 0) {
    return 0;
  }
  
  p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((int64_t)_dig_P9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t)_dig_P8) * p) >> 19;
  
  p = ((p + var1 + var2) >> 8) + (((int64_t)_dig_P7) << 4);
  return (float)p / 25600.0; // Convert to hPa
}

// Setup Mode Functions
void checkFirstBoot() {
  prefs.begin(PREF_NS, true); // Read-only
  String savedSSID = prefs.getString("ssid", "");
  prefs.end();
  
  if (savedSSID.length() == 0 || savedSSID == defSSID) {
    setupMode = true;
    SerialPrintTR("[SETUP MODE] Starting WiFi AP...");
  }
}

void startSetupMode() {
  setupModeActive = true;
  
  // Stop WiFi if connected
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect(true);
    delay(100);
  }
  
  // Start AP mode
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SETUP_AP_SSID, SETUP_AP_PASS);
  
  delay(500);
  
  IPAddress apIP = WiFi.softAPIP();
  
  Serial.print(TR("[SETUP MODE] SSID: "));
  Serial.println(SETUP_AP_SSID);
  Serial.print("IP: ");
  Serial.println(apIP);
  SerialPrintTR("[SETUP MODE] Connect to configure");
  
  // Start DNS server for captive portal
  dnsServer.start(53, "*", apIP);
  
  // Setup web handlers
  setupModeHandlers();
  
  // Start web server
  server.begin();
  
  // Display on screen
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_t_cyrillic);
  u8g2.setCursor(0, 15);
  u8g2.print(TR("[SETUP MODE] SSID: "));
  u8g2.setCursor(0, 30);
  u8g2.print(SETUP_AP_SSID);
  u8g2.setCursor(0, 45);
  u8g2.print("IP: ");
  u8g2.print(apIP);
  u8g2.sendBuffer();
}

void stopSetupMode() {
  setupModeActive = false;
  setupMode = false;
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  SerialPrintTR("[SETUP MODE] Exiting...");
}

void handleSetupModeLED() {
  if (setupModeActive) {
    if (millis() - setupLedBlink >= SETUP_LED_INTERVAL) {
      setupLedBlink = millis();
      setupLedState = !setupLedState;
      if (setupLedState) {
        pixel.setPixelColor(0, pixel.Color(255, 255, 255)); // White
      } else {
        pixel.setPixelColor(0, pixel.Color(0, 0, 0)); // Off
      }
      pixel.show();
    }
  }
}

void setupModeHandlers() {
  // Serve setup page
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", SETUP_PAGE_HTML);
  });
  
  // Handle captive portal detection
  server.on("/generate_204", HTTP_GET, []() {
    server.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
    server.send(302, "text/plain", "");
  });
  
  server.on("/hotspot-detect.html", HTTP_GET, []() {
    server.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
    server.send(302, "text/plain", "");
  });
  
  // Save configuration
  server.on("/setup/save", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      String body = server.arg("plain");
      
      // Parse JSON manually (simple parser)
      String ssid = "";
      String password = "";
      String language = "en";
      String theme = "light";
      String datetime = "";
      int tz = 3;
      int dst = 0;
      String ntp = "pool.ntp.org";
      String city = "Moscow";
      
      // Extract values from JSON
      int idx = body.indexOf("\"ssid\":\"");
      if (idx >= 0) {
        idx += 8;
        int end = body.indexOf("\"", idx);
        ssid = body.substring(idx, end);
      }
      
      idx = body.indexOf("\"password\":\"");
      if (idx >= 0) {
        idx += 12;
        int end = body.indexOf("\"", idx);
        password = body.substring(idx, end);
      }
      
      idx = body.indexOf("\"language\":\"");
      if (idx >= 0) {
        idx += 12;
        int end = body.indexOf("\"", idx);
        language = body.substring(idx, end);
      }
      
      idx = body.indexOf("\"theme\":\"");
      if (idx >= 0) {
        idx += 9;
        int end = body.indexOf("\"", idx);
        theme = body.substring(idx, end);
      }
      
      idx = body.indexOf("\"datetime\":\"");
      if (idx >= 0) {
        idx += 12;
        int end = body.indexOf("\"", idx);
        datetime = body.substring(idx, end);
      }
      
      idx = body.indexOf("\"tz\":\"");
      if (idx >= 0) {
        idx += 6;
        int end = body.indexOf("\"", idx);
        tz = body.substring(idx, end).toInt();
      }
      
      idx = body.indexOf("\"dst\":\"");
      if (idx >= 0) {
        idx += 7;
        int end = body.indexOf("\"", idx);
        dst = body.substring(idx, end).toInt();
      }
      
      idx = body.indexOf("\"ntp\":\"");
      if (idx >= 0) {
        idx += 7;
        int end = body.indexOf("\"", idx);
        ntp = body.substring(idx, end);
      }
      
      idx = body.indexOf("\"city\":\"");
      if (idx >= 0) {
        idx += 8;
        int end = body.indexOf("\"", idx);
        city = body.substring(idx, end);
      }
      
      // Save to NVS
      prefs.begin(PREF_NS, false);
      prefs.putString("ssid", ssid);
      prefs.putString("pass", password);
      prefs.putString("ntp", ntp);
      prefs.putLong("gmtOffset", tz * 3600);
      prefs.putLong("dstOffset", dst * 3600);
      prefs.putString("city", city);
      prefs.end();
      
      prefs.begin(THEME_PREF_NS, false);
      prefs.putString("theme", theme);
      prefs.putString("language", language);
      prefs.end();
      
      // Set manual time if provided
      if (datetime.length() > 0) {
        // Parse datetime: YYYY-MM-DDTHH:MM
        int year = datetime.substring(0, 4).toInt();
        int month = datetime.substring(5, 7).toInt();
        int day = datetime.substring(8, 10).toInt();
        int hour = datetime.substring(11, 13).toInt();
        int minute = datetime.substring(14, 16).toInt();
        
        timeinfo.tm_year = year - 1900;
        timeinfo.tm_mon = month - 1;
        timeinfo.tm_mday = day;
        timeinfo.tm_hour = hour;
        timeinfo.tm_min = minute;
        timeinfo.tm_sec = 0;
        
        time_t t = mktime(&timeinfo);
        struct timeval now = { .tv_sec = t };
        settimeofday(&now, NULL);
        timeValid = true;
      }
      
      server.send(200, "application/json", "{\"status\":\"ok\"}");
      
      Serial.println(TR("[SETUP MODE] Configuration saved, rebooting..."));
      delay(1000);
      ESP.restart();
    } else {
      server.send(400, "application/json", "{\"status\":\"error\"}");
    }
  });
  
  // Catch-all for captive portal
  server.onNotFound([]() {
    server.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
    server.send(302, "text/plain", "");
  });
}

void setup() {
  // Initialize NVS first - required after full flash erase
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // NVS partition was truncated and needs to be erased
    nvs_flash_erase();
    err = nvs_flash_init();
  }
  
  pinMode(BOOT_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  
  #if BUZZER_TYPE == BUZZER_PASSIVE && LOW_LEVEL_TRIGGER
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, HIGH);
  #else
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
  #endif

  pixel.begin();
  pixel.setBrightness(50);
  pixel.clear();
  pixel.show();

  Serial.begin(115200);
  Serial.setRxBufferSize(512);
  Serial.setTxBufferSize(512);
  // Small delay for Serial to stabilize
  unsigned long serialWait = millis();
  while (millis() - serialWait < 100) { /* wait */ }

 
  Serial.println("\r\n\r\n========================================");
  Serial.println("   ESP32-S3-Z-Clock v1.2.1");
  Serial.println("   (c) CheshirCa 2026");
  Serial.println("========================================");
  SerialPrintTR("Initializing...");
  SerialPrintTR("NVS initialized OK");

  u8g2.begin();
  u8g2.setBusClock(400000);
  u8g2.enableUTF8Print();
  
  loadDisplaySettings();
  setDisplayBrightness(displayBrightness);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14_tf);
  u8g2.setCursor(40, 35);
  u8g2.print("INIT OK");
  u8g2.sendBuffer();
  unsigned long waitStart = millis();
  while (millis() - waitStart < 500) { /* wait */ }

  loadConfigFromNVS();
  loadAllAlarmsFromNVS();
  
  // Check if this is first boot or setup mode requested
  checkFirstBoot();
  
  if (setupMode) {
    // Start setup mode immediately
    loadThemeLanguageSettings(); // Load language first for translated messages
    startSetupMode();
    return; // Exit setup, will run in loop
  }
  
  Serial.print(TR("WiFi SSID: "));
  Serial.println(wifiSSID);
  if (wifiSSID == defSSID || wifiSSID.length() == 0) {
    SerialPrintTR("WARNING: Using default WiFi settings!");
    SerialPrintTR("Configure WiFi via Serial commands or web interface");
  }
  
  #if BUZZER_TYPE == BUZZER_PASSIVE
  loadMelodySettings();
  parseMelody(alarmMelody, alarmNotes, alarmNoteCount, 50);
  parseMelody(timerMelody, timerNotes, timerNoteCount, 50);
  #endif
  
  loadNightModeSettings();
  loadWiFiModeSettings();
  loadThemeLanguageSettings();

  showSplash();
  
  WiFi.mode(WIFI_STA);
  waitStart = millis();
  while (millis() - waitStart < 200) { /* wait */ }
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer.c_str());
  waitStart = millis();
  while (millis() - waitStart < 200) { /* wait */ }
  
  connectWiFi();
  
  if (WiFi.status() == WL_CONNECTED) {
    syncTime();
    waitStart = millis();
    while (millis() - waitStart < 100) { /* wait */ }
    updateWeather();
    waitStart = millis();
    while (millis() - waitStart < 100) { /* wait */ }
    
    // Initialize sync timestamps
    lastNtpSync = millis();
    lastWeatherSync = millis();
  }
  
  // If Smart WiFi mode, disconnect after initial sync
  if (wifiMode == WIFI_SMART) {
    WiFi.disconnect(true, true);
    delay(100);
    WiFi.mode(WIFI_OFF);
    SerialPrintTR("[Smart WiFi] Initial sync complete, WiFi OFF");
  }
  
  updateClockStrings();
  
  i2cMutex = xSemaphoreCreateMutex();
  if (i2cMutex == NULL) {
    SerialPrintTR("Failed to create I2C mutex!");
  }
  
  Wire1.begin(SDA_PIN, SCL_PIN);
  delay(100);  // Give I2C bus time to stabilize
  
  SerialPrintTR("Initializing sensors...");
  
  if (i2cMutex != NULL) {
    bool acquired = false;
    for (int retry = 0; retry < I2C_MAX_RETRIES && !acquired; retry++) {
      if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) == pdTRUE) {
        acquired = true;
        
        // AHT20 - try multiple times with delays
        for (int attempt = 0; attempt < 3 && !ahtFound; attempt++) {
          if (attempt > 0) {
            delay(50);  // Wait before retry
          }
          if (aht.begin(&Wire1)) {
            ahtFound = true;
            SerialPrintTR("AHT20: OK (0x38 on Wire1)");
            break;
          }
        }
        if (!ahtFound) {
          SerialPrintTR("AHT20: Not found after retries");
        }
        
        // BMP280 direct access
        bmpFound = BMP280_begin();
        if (bmpFound) {
          SerialPrintTR("BMP280: OK (direct access)");
        } else {
          SerialPrintTR("BMP280: Not found");
        }
        
        xSemaphoreGive(i2cMutex);
      } else {
        Serial.printf("I2C mutex timeout during init (retry %d/%d)\n", retry + 1, I2C_MAX_RETRIES);
        if (retry == I2C_MAX_RETRIES - 1) {
          SerialPrintTR("Failed to acquire I2C mutex after retries");
        }
      }
    }
  }
  
  if (ahtFound || bmpFound) {
    readSensorData();
  }
  
  waitStart = millis();
  while (millis() - waitStart < 100) { /* wait */ }
  setupWebServer();
  server.begin();
  waitStart = millis();
  while (millis() - waitStart < 100) { /* wait */ }
  
  Serial.println("\r\n========================================");
  SerialPrintTR("Setup complete!");
  Serial.println("========================================");
  SerialPrintTR("Web server: RUNNING");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(TR("Web interface: http://"));
    Serial.println(WiFi.localIP());
    Serial.print(TR("WiFi Signal: "));
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    SerialPrintTR("WiFi: DISCONNECTED");
    SerialPrintTR("Will auto-connect for sync or web access");
  }
  Serial.println("========================================");
  Serial.print("\r\n");
  SerialPrintTR("Available commands:");
  SerialPrintTR("  status     - Show system status");
  SerialPrintTR("  alarm list - Show all alarms");
  SerialPrintTR("  wifi ...   - WiFi management");
  SerialPrintTR("  help       - Show all commands");
  Serial.println("========================================\r\n");
  Serial.print("> ");
}

void readSensorData() {
  if (readingSensors || updatingWeather) return;
  readingSensors = true;

  if (i2cMutex != NULL) {
    bool acquired = false;
    for (int retry = 0; retry < I2C_MAX_RETRIES && !acquired; retry++) {
      if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) == pdTRUE) {
        acquired = true;
        
        if (ahtFound) {
          aht.getEvent(&humidity, &temp);
          temperature = temp.temperature;
          humidityVal = humidity.relative_humidity;
        }
        
        if (bmpFound) {
          float bmpTemp = readTemperatureBMP280();
          pressure = readPressureBMP280();
          
          if (!ahtFound) {
            temperature = bmpTemp;
          }
        }
        
        xSemaphoreGive(i2cMutex);
      } else if (retry < I2C_MAX_RETRIES - 1) {
        // Brief delay before retry
        unsigned long retryWait = millis();
        while (millis() - retryWait < 10) { /* wait */ }
      }
    }
  }
  
  lastSensorRead = millis();
  readingSensors = false;
}

void loadConfigFromNVS() {
  prefs.begin(PREF_NS, true);
  if (prefs.isKey("ssid") && !prefs.getString("ssid").isEmpty()) {
    wifiSSID = prefs.getString("ssid");
    wifiPASS = prefs.getString("pass");
    ntpServer = prefs.getString("ntp", defNTP);
    gmtOffset_sec = prefs.getLong("gmtOffset", defGMTOffset);
    daylightOffset_sec = prefs.getLong("daylightOffset", defDaylightOffset);
    cityName = prefs.getString("city", defCity);
  } else {
    wifiSSID = defSSID;
    wifiPASS = defPASS;
    ntpServer = defNTP;
    gmtOffset_sec = defGMTOffset;
    daylightOffset_sec = defDaylightOffset;
    cityName = defCity;
  }
  prefs.end();
}

void saveConfigToNVS() {
  prefs.begin(PREF_NS, false);
  prefs.putString("ssid", wifiSSID);
  prefs.putString("pass", wifiPASS);
  prefs.putString("ntp", ntpServer);
  prefs.putLong("gmtOffset", gmtOffset_sec);
  prefs.putLong("daylightOffset", daylightOffset_sec);
  prefs.putString("city", cityName);
  prefs.end();
}

void loadAlarmFromNVS(int index) {
  if (index < 0 || index >= MAX_ALARMS) return;
  char ns[10];
  sprintf(ns, "alarm%d", index);
  prefs.begin(ns, true);
  if (prefs.isKey("active")) {
    alarms[index].active = prefs.getBool("active", false);
    alarms[index].year = prefs.getInt("year", 0);
    alarms[index].month = prefs.getInt("month", 0);
    alarms[index].day = prefs.getInt("day", 0);
    alarms[index].weekdays = prefs.getInt("weekdays", 0);
    alarms[index].hour = prefs.getInt("hour", 0);
    alarms[index].minute = prefs.getInt("minute", 0);
    alarms[index].repeat = prefs.getBool("repeat", false);
    alarms[index].saved = true;
    String text = prefs.getString("text", "");
    text.toCharArray(alarms[index].text, sizeof(alarms[index].text));
    String mel = prefs.getString("melody", "");
    mel.toCharArray(alarms[index].melody, sizeof(alarms[index].melody));
    alarms[index].useDefaultMelody = prefs.getBool("useDef", true);
  } else {
    memset(&alarms[index], 0, sizeof(alarms[index]));
    alarms[index].useDefaultMelody = true;
  }
  prefs.end();
  updateLEDIndicator();
}

void saveAlarmToNVS(int index) {
  if (index < 0 || index >= MAX_ALARMS) return;
  char ns[10];
  sprintf(ns, "alarm%d", index);
  prefs.begin(ns, false);
  prefs.putBool("active", alarms[index].active);
  prefs.putInt("year", alarms[index].year);
  prefs.putInt("month", alarms[index].month);
  prefs.putInt("day", alarms[index].day);
  prefs.putInt("weekdays", alarms[index].weekdays);
  prefs.putInt("hour", alarms[index].hour);
  prefs.putInt("minute", alarms[index].minute);
  prefs.putBool("repeat", alarms[index].repeat);
  prefs.putString("text", String(alarms[index].text));
  prefs.putString("melody", String(alarms[index].melody));
  prefs.putBool("useDef", alarms[index].useDefaultMelody);
  prefs.end();
  alarms[index].saved = true;
  updateLEDIndicator();
}

void clearAlarmFromNVS(int index) {
  if (index < 0 || index >= MAX_ALARMS) return;
  char ns[10];
  sprintf(ns, "alarm%d", index);
  prefs.begin(ns, false);
  prefs.clear();
  prefs.end();
  alarms[index] = Alarm();
  alarms[index].saved = false;
  updateLEDIndicator();
}

void loadAllAlarmsFromNVS() {
  for (int i = 0; i < MAX_ALARMS; i++) {
    loadAlarmFromNVS(i);
  }
}

void saveAllAlarmsToNVS() {
  for (int i = 0; i < MAX_ALARMS; i++) {
    saveAlarmToNVS(i);
  }
}

void loadDisplaySettings() {
  prefs.begin(DISPLAY_PREF_NS, true);
  displayBrightness = prefs.getInt("brightness", defBrightness);
  prefs.end();
}

void saveDisplaySettings() {
  prefs.begin(DISPLAY_PREF_NS, false);
  prefs.putInt("brightness", displayBrightness);
  prefs.end();
}

void loadMelodySettings() {
  #if BUZZER_TYPE == BUZZER_PASSIVE
  prefs.begin(MELODY_PREF_NS, true);
  alarmMelody = prefs.getString("alarm", defAlarmMelody);
  timerMelody = prefs.getString("timer", defTimerMelody);
  prefs.end();
  #endif
}

void saveMelodySettings() {
  #if BUZZER_TYPE == BUZZER_PASSIVE
  prefs.begin(MELODY_PREF_NS, false);
  prefs.putString("alarm", alarmMelody);
  prefs.putString("timer", timerMelody);
  prefs.end();
  #endif
}

void loadNightModeSettings() {
  prefs.begin(NIGHT_PREF_NS, true);
  nightModeEnabled = prefs.getBool("enabled", false);
  nightStartHour = prefs.getInt("startHour", 23);
  nightStartMinute = prefs.getInt("startMin", 0);
  nightEndHour = prefs.getInt("endHour", 7);
  nightEndMinute = prefs.getInt("endMin", 0);
  nightBrightness = prefs.getInt("brightness", 10);
  prefs.end();
  
  Serial.print(TR("[Night Mode] "));
  Serial.println(nightModeEnabled ? "ENABLED" : "DISABLED");
  if (nightModeEnabled) {
    Serial.printf("[Night Mode] Time: %02d:%02d - %02d:%02d, Brightness: %d\r\n",
            nightStartHour, nightStartMinute, nightEndHour, nightEndMinute, nightBrightness);
  }
}

void saveNightModeSettings() {
  prefs.begin(NIGHT_PREF_NS, false);
  prefs.putBool("enabled", nightModeEnabled);
  prefs.putInt("startHour", nightStartHour);
  prefs.putInt("startMin", nightStartMinute);
  prefs.putInt("endHour", nightEndHour);
  prefs.putInt("endMin", nightEndMinute);
  prefs.putInt("brightness", nightBrightness);
  prefs.end();
}

void loadWiFiModeSettings() {
  prefs.begin(WIFI_PREF_NS, true);
  wifiMode = (WiFiMode)prefs.getInt("mode", WIFI_ALWAYS_ON);  // Default to always on
  wifiAutoOffTimeout = prefs.getULong("autoOff", 10 * 60 * 1000);
  ntpSyncInterval = prefs.getULong("ntpSync", 60 * 60 * 1000);
  weatherSyncInterval = prefs.getULong("weatherSync", 30 * 60 * 1000);
  prefs.end();
  
  Serial.print("[WiFi] Mode loaded: ");
  Serial.println(wifiMode == WIFI_ALWAYS_ON ? "ALWAYS ON" : "SMART WiFi");
  if (wifiMode == WIFI_SMART) {
    Serial.printf("[WiFi] Auto-off: %lu min, NTP: %lu min, Weather: %lu min\r\n", 
            wifiAutoOffTimeout / 60000, ntpSyncInterval / 60000, weatherSyncInterval / 60000);
  }
}

void saveWiFiModeSettings() {
  prefs.begin(WIFI_PREF_NS, false);
  prefs.putInt("mode", (int)wifiMode);
  prefs.putULong("autoOff", wifiAutoOffTimeout);
  prefs.putULong("ntpSync", ntpSyncInterval);
  prefs.putULong("weatherSync", weatherSyncInterval);
  prefs.end();
}

void loadThemeLanguageSettings() {
  prefs.begin(THEME_PREF_NS, true);
  currentTheme = prefs.getString("theme", "light");
  currentLanguage = prefs.getString("language", "en");
  prefs.end();
  
  Serial.println("Theme & Language settings loaded from NVS:");
  Serial.print("  Theme: ");
  Serial.println(currentTheme);
  Serial.print("  Language: ");
  Serial.println(currentLanguage);
}

void saveThemeLanguageSettings() {
  prefs.begin(THEME_PREF_NS, false);
  prefs.putString("theme", currentTheme);
  prefs.putString("language", currentLanguage);
  prefs.end();
  
  Serial.println("Theme & Language settings saved to NVS");
}

void checkNightMode() {
  if (!nightModeEnabled || !timeValid) {
    nightModeActive = false;
    return;
  }
  
  int currentMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
  int startMinutes = nightStartHour * 60 + nightStartMinute;
  int endMinutes = nightEndHour * 60 + nightEndMinute;
  
  bool inNightMode = false;
  if (startMinutes < endMinutes) {
    // Normal case: 23:00 - 07:00 crosses midnight
    inNightMode = (currentMinutes >= startMinutes && currentMinutes < endMinutes);
  } else {
    // Edge case: time range doesn't cross midnight
    inNightMode = (currentMinutes >= startMinutes || currentMinutes < endMinutes);
  }
  
  if (inNightMode != nightModeActive) {
    nightModeActive = inNightMode;
    setDisplayBrightness(inNightMode ? nightBrightness : displayBrightness);
  }
}

void eraseNVS() {
  prefs.begin(PREF_NS, false);
  prefs.clear();
  prefs.end();
  for (int i = 0; i < MAX_ALARMS; i++) {
    clearAlarmFromNVS(i);
  }
  prefs.begin(DISPLAY_PREF_NS, false);
  prefs.clear();
  prefs.end();
  #if BUZZER_TYPE == BUZZER_PASSIVE
  prefs.begin(MELODY_PREF_NS, false);
  prefs.clear();
  prefs.end();
  #endif
  prefs.begin(NIGHT_PREF_NS, false);
  prefs.clear();
  prefs.end();
  prefs.begin(WIFI_PREF_NS, false);
  prefs.clear();
  prefs.end();
  prefs.begin(THEME_PREF_NS, false);
  prefs.clear();
  prefs.end();
}

void setDisplayBrightness(int brightness) {
  if (brightness < 0) brightness = 0;
  if (brightness > 255) brightness = 255;
  displayBrightness = brightness;
  u8g2.setContrast(brightness);
}

void updateClockStrings() {
  if (getLocalTime(&timeinfo) && timeinfo.tm_year > 120) {
    snprintf(hhStr, sizeof(hhStr), "%02d", timeinfo.tm_hour);
    snprintf(mmStr, sizeof(mmStr), "%02d", timeinfo.tm_min);
    snprintf(dateStr, sizeof(dateStr), "%02d.%02d.%04d",
             timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
    int wday = timeinfo.tm_wday;
    if (wday >= 0 && wday < 7) {
      strcpy(weekdayStr, weekdaysRU[wday]);
    } else {
      strcpy(weekdayStr, "---");
    }
  }
}

void drawClock() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_logisoso38_tn);
  const char* ref = "88:88";
  int refW = u8g2.getStrWidth(ref);
  int refX = X_OFF + (DISP_W - refW) / 2;
  int refY = Y_OFF + 40;
  int colonOffset = u8g2.getStrWidth("88");
  int hhX = refX;
  int colonX = refX + colonOffset;
  int mmX = colonX + u8g2.getStrWidth(":");
  u8g2.setCursor(hhX, refY);
  u8g2.print(hhStr);
  u8g2.setCursor(colonX, refY);
  u8g2.print(colonVisible ? ":" : " ");
  u8g2.setCursor(mmX, refY);
  u8g2.print(mmStr);
  String ds = String(dateStr);
  bool anyActive = false;
  for (int i = 0; i < MAX_ALARMS; i++) {
    if (alarms[i].active) {
      anyActive = true;
      break;
    }
  }
  if (anyActive) ds += " A";
  if (timerActive) ds += " T";
  u8g2.setFont(u8g2_font_8x13_t_cyrillic);
  int dw = u8g2.getStrWidth(ds.c_str());
  int dx = X_OFF + (DISP_W - dw) / 2;
  u8g2.setCursor(dx, 16);
  u8g2.print(ds);
  u8g2.sendBuffer();
}

void drawAlarmOrTimer(const char* txt) {
  u8g2.clearBuffer();
  String stopMessage = "Press BOOT!";
  u8g2.setFont(u8g2_font_6x13_t_cyrillic);
  int stopW = u8g2.getUTF8Width(stopMessage.c_str());
  int stopX = (DISP_W - stopW) / 2;
  u8g2.drawUTF8(stopX, 12, stopMessage.c_str());
  String userText = (txt && txt[0]) ? String(txt) : "ALARM";
  const int blueAreaY = BLUE_ZONE_MIN;
  const int blueAreaHeight = BLUE_ZONE_MAX - BLUE_ZONE_MIN;
  const int blueAreaWidth = DISP_W;
  TextLine lines[10];
  int lineCount = 0;
  bool fitted = false;
  const uint8_t* selectedFont = nullptr;
  for (int i = 0; i < fontListSize; i++) {
    if (tryFitText(fontList[i], userText, blueAreaWidth, blueAreaHeight, lines, lineCount, 10)) {
      selectedFont = fontList[i];
      fitted = true;
      break;
    }
  }
  if (!fitted) {
    u8g2.setFont(u8g2_font_4x6_t_cyrillic);
    String errorMsg = "TEXT TOO LONG!";
    int errW = u8g2.getUTF8Width(errorMsg.c_str());
    int errX = (DISP_W - errW) / 2;
    int errY = blueAreaY + (blueAreaHeight / 2);
    u8g2.drawUTF8(errX, errY, errorMsg.c_str());
  } else {
    u8g2.setFont(selectedFont);
    int lineHeight = u8g2.getMaxCharHeight();
    int totalHeight = lineCount * lineHeight;
    int startY = blueAreaY + (blueAreaHeight - totalHeight) / 2 + lineHeight;
    for (int i = 0; i < lineCount; i++) {
      int lineX = (DISP_W - lines[i].width) / 2;
      int lineY = startY + i * lineHeight;
      u8g2.drawUTF8(lineX, lineY, lines[i].text.c_str());
    }
  }
  u8g2.sendBuffer();
}

void drawInfoScreen1() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_t_cyrillic);
  int y = 10;
  u8g2.setCursor(28, y);
  u8g2.print("INFO 1/4");
  y += 15;
  u8g2.setCursor(X_OFF, y);
  u8g2.printf("Day: %s", weekdayStr);
  y += 12;
  u8g2.setCursor(X_OFF, y);
  int activeCount = 0;
  for (int i = 0; i < MAX_ALARMS; i++) {
    if (alarms[i].active) activeCount++;
  }
  if (activeCount > 0) {
    u8g2.printf("Alarm: %d ON", activeCount);
  } else {
    u8g2.print("Alarm: OFF");
  }
  y += 12;
  u8g2.setCursor(X_OFF, y);
  if (timerActive) {
    uint64_t elapsed = esp_timer_get_time() - timerStartUs;
    uint64_t remaining = (elapsed >= timerDurationUs) ? 0 : (timerDurationUs - elapsed);
    int secRemaining = (remaining + 500000) / 1000000;
    u8g2.printf("Timer: %d sec", secRemaining);
  } else {
    u8g2.print("Timer: OFF");
  }
  y += 12;
  u8g2.setCursor(X_OFF, y);
  u8g2.printf("WiFi: %s", WiFi.status() == WL_CONNECTED ? "ON" : "OFF");
  u8g2.sendBuffer();
}

void drawInfoScreen2() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_t_cyrillic);
  int y = 10;
  u8g2.setCursor(28, y);
  u8g2.print("INFO 2/4");
  y += 15;
  u8g2.setCursor(X_OFF, y);
  String ssidDisplay = wifiSSID;
  if (ssidDisplay.length() > 14) {
    ssidDisplay = ssidDisplay.substring(0, 14) + "...";
  }
  u8g2.printf("SSID: %s", ssidDisplay.c_str());
  y += 12;
  u8g2.setCursor(X_OFF, y);
  if (WiFi.status() == WL_CONNECTED) {
    IPAddress ip = WiFi.localIP();
    u8g2.printf("IP: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  } else {
    u8g2.print("IP: No WiFi");
  }
  y += 12;
  u8g2.setCursor(X_OFF, y);
  u8g2.printf("Time: %s", timeValid ? "SYNC" : "NO SYNC");
  y += 12;
  u8g2.setCursor(X_OFF, y);
  u8g2.printf("RAM: %d KB", esp_get_free_heap_size() / 1024);
  u8g2.sendBuffer();
}

void drawInfoScreen3() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_t_cyrillic);
  u8g2.setCursor(28, 10);
  u8g2.print(TR("WEATHER"));
  if (weatherData.length() == 0 || weatherData.indexOf("No WiFi") >= 0 || 
      weatherData.indexOf("Error") >= 0) {
    u8g2.setFont(u8g2_font_5x8_t_cyrillic);
    String msg = weatherData.length() > 0 ? weatherData : TR("No data");
    int msgW = u8g2.getUTF8Width(msg.c_str());
    int msgX = (DISP_W - msgW) / 2;
    u8g2.drawUTF8(msgX, 40, msg.c_str());
    u8g2.sendBuffer();
    return;
  }
  const int areaY = 22;
  const int areaHeight = 42;
  const int areaWidth = DISP_W;
  String weatherText = "";
  int startPos = 0;
  int maxLines = 4;
  for (int i = 0; i < maxLines; i++) {
    int endPos = weatherData.indexOf('\n', startPos);
    if (endPos == -1) endPos = weatherData.length();
    String line = weatherData.substring(startPos, endPos);
    line.trim();
    if (line.length() > 0) {
      if (weatherText.length() > 0) weatherText += "\n";
      weatherText += line;
    }
    startPos = endPos + 1;
    if (startPos >= weatherData.length()) break;
  }
  if (weatherText.length() > 100) {
    weatherText = weatherText.substring(0, 100) + "...";
  }
  TextLine lines[10];
  int lineCount = 0;
  bool fitted = false;
  const uint8_t* selectedFont = nullptr;
  for (int i = 0; i < fontListSize; i++) {
    if (tryFitText(fontList[i], weatherText, areaWidth, areaHeight, lines, lineCount, 10)) {
      selectedFont = fontList[i];
      fitted = true;
      break;
    }
  }
  if (!fitted) {
    selectedFont = u8g2_font_4x6_t_cyrillic;
    u8g2.setFont(selectedFont);
    String shortText = "";
    startPos = 0;
    int linesAdded = 0;
    while (startPos < weatherText.length() && linesAdded < 6) {
      int endPos = weatherText.indexOf('\n', startPos);
      if (endPos == -1) endPos = weatherText.length();
      String line = weatherText.substring(startPos, endPos);
      line.trim();
      if (line.length() > 20) {
        line = line.substring(0, 20) + "..";
      }
      if (line.length() > 0) {
        lines[linesAdded].text = line;
        lines[linesAdded].width = u8g2.getUTF8Width(line.c_str());
        linesAdded++;
      }
      startPos = endPos + 1;
    }
    lineCount = linesAdded;
  }
  u8g2.setFont(selectedFont);
  int lineHeight = u8g2.getMaxCharHeight();
  int totalHeight = lineCount * lineHeight;
  int startY = areaY + (areaHeight - totalHeight) / 2 + lineHeight;
  if (startY - lineHeight < BLUE_ZONE_MIN) {
    startY = BLUE_ZONE_MIN + lineHeight;
  }
  for (int i = 0; i < lineCount; i++) {
    int lineX = (DISP_W - lines[i].width) / 2;
    int lineY = startY + i * lineHeight;
    if (lineY >= BLUE_ZONE_MIN && lineY <= BLUE_ZONE_MAX) {
      u8g2.drawUTF8(lineX, lineY, lines[i].text.c_str());
    }
  }
  u8g2.sendBuffer();
}

void drawInfoScreen4() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_t_cyrillic);

  int y = 10;

  // Header in yellow zone - ИЗМЕНИТЬ на INFO 4/4
  u8g2.setCursor(28, y);
  u8g2.print("INFO 4/4");
  y += 15;

  // Info in blue zone
  u8g2.setCursor(X_OFF, y);
  u8g2.print("SENSOR DATA");
  y += 15;

  if (ahtFound || bmpFound) {
    // Temperature
    u8g2.setCursor(X_OFF, y);
    u8g2.printf("Temp: %.1f C", temperature);
    y += 12;
    
    // Humidity (только если есть AHT20)
    if (ahtFound) {
      u8g2.setCursor(X_OFF, y);
      u8g2.printf("Hum: %.1f %%", humidityVal);
      y += 12;
    }
    
    // Pressure (только если есть BMP280)
    if (bmpFound) {
      u8g2.setCursor(X_OFF, y);
      u8g2.printf("Press: %.1f hPa", pressure);
      y += 12;
    }
  } else {
    u8g2.setCursor(X_OFF, y);
    u8g2.print("No sensors found");
  }

  u8g2.sendBuffer();
}

void showSplash() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_t_cyrillic);
  u8g2.setCursor(4, 12);
  u8g2.print("(c) CheshirCa 2026");
  u8g2.setFont(u8g2_font_unifont_t_cyrillic);
  u8g2.setCursor(X_OFF + 4, Y_OFF + 14);
  u8g2.print("ESP32-S3-Zero");
  u8g2.setCursor(X_OFF + 16, Y_OFF + 32);
  u8g2.print("ALARM CLOCK");
  u8g2.sendBuffer();
  unsigned long splashStart = millis();
  while (millis() - splashStart < 1500) { /* wait */ }
}

void splitWords(String text, String* words, int &wordCount, int maxWords) {
  wordCount = 0;
  int start = 0;
  for (int i = 0; i <= text.length() && wordCount < maxWords; i++) {
    if (i == text.length() || text[i] == ' ' || text[i] == '\n') {
      if (i > start) {
        words[wordCount++] = text.substring(start, i);
      }
      start = i + 1;
    }
  }
}

bool tryFitText(const uint8_t* font, String text, int areaWidth, int areaHeight, 
                TextLine* lines, int &lineCount, int maxLines) {
  u8g2.setFont(font);
  String words[50];
  int wordCount = 0;
  splitWords(text, words, wordCount, 50);
  if (wordCount == 0) {
    lines[0].text = "";
    lines[0].width = 0;
    lineCount = 1;
    return true;
  }
  int lineHeight = u8g2.getMaxCharHeight();
  lineCount = 0;
  String currentLine = "";
  int currentWidth = 0;
  for (int i = 0; i < wordCount && lineCount < maxLines; i++) {
    String word = words[i];
    int wordWidth = u8g2.getUTF8Width(word.c_str());
    int spaceWidth = u8g2.getUTF8Width(" ");
    if (wordWidth > areaWidth) {
      return false;
    }
    int newWidth = currentWidth;
    if (currentLine.length() > 0) {
      newWidth += spaceWidth + wordWidth;
    } else {
      newWidth = wordWidth;
    }
    if (newWidth <= areaWidth) {
      if (currentLine.length() > 0) {
        currentLine += " ";
      }
      currentLine += word;
      currentWidth = newWidth;
    } else {
      if (lineCount >= maxLines) {
        return false;
      }
      lines[lineCount].text = currentLine;
      lines[lineCount].width = currentWidth;
      lineCount++;
      currentLine = word;
      currentWidth = wordWidth;
    }
  }
  if (currentLine.length() > 0 && lineCount < maxLines) {
    lines[lineCount].text = currentLine;
    lines[lineCount].width = currentWidth;
    lineCount++;
  }
  int totalHeight = lineCount * lineHeight;
  if (totalHeight > areaHeight) {
    return false;
  }
  return true;
}

String urlEncode(const String& str) {
  String encoded = "";
  char c;
  char code0;
  char code1;
  for (unsigned int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encoded += "%20";
    } else if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      encoded += '%';
      encoded += code0;
      encoded += code1;
    }
  }
  return encoded;
}

void connectWiFi() {
  if (wifiSSID == defSSID || wifiSSID.length() == 0) {
    return;
  }
  WiFi.disconnect(true);
  unsigned long waitStart = millis();
  while (millis() - waitStart < 100) { /* wait */ }
  WiFi.begin(wifiSSID.c_str(), wifiPASS.c_str());
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > 15000) {
      WiFi.disconnect(true);
      return;
    }
    waitStart = millis();
    while (millis() - waitStart < 200) { /* wait */ }
  }
}

void syncTime() {
  timeValid = false;
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer.c_str());
  for (int i = 0; i < 30; i++) {
    if (getLocalTime(&timeinfo) && timeinfo.tm_year > 120) {
      timeValid = true;
      return;
    }
    unsigned long waitStart = millis();
    while (millis() - waitStart < 500) { /* wait */ }
  }
}

void setManualTime(String s) {
  struct tm t {};
  if (sscanf(s.c_str(), "%d-%d-%d %d:%d:%d",
             &t.tm_year, &t.tm_mon, &t.tm_mday,
             &t.tm_hour, &t.tm_min, &t.tm_sec) == 6) {
    t.tm_year -= 1900;
    t.tm_mon -= 1;
    time_t tt = mktime(&t);
    struct timeval now = { tt, 0 };
    settimeofday(&now, nullptr);
    timeValid = true;
    updateClockStrings();
  }
}

String getSimpleWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    return TR("No WiFi");
  }
  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(8000);
  String encodedCity = urlEncode(cityName);
  String url = "http://wttr.in/" + encodedCity + "?format=%t+%C&lang=" + currentLanguage;
  
  bool success = http.begin(url);
  if (!success) {
    return TR("Connection failed");
  }
  
  http.setUserAgent("curl/7.64.1");
  String result = "";
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK || httpCode == 200) {
    String payload = http.getString();
    payload.replace("\r", "");
    payload.trim();
    int spacePos = payload.indexOf(' ');
    if (spacePos > 0) {
      String temp = payload.substring(0, spacePos);
      String condition = payload.substring(spacePos + 1);
      result = temp + " " + condition;
    } else {
      result = payload;
    }
  } else {
    result = "Error " + String(httpCode);
  }
  http.end();
  return result;
}

void updateWeather() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (updatingWeather) return;
  
  updatingWeather = true;
  weatherData = getSimpleWeather();
  lastWeatherUpdate = millis();
  updatingWeather = false;
}

#if BUZZER_TYPE == BUZZER_PASSIVE
void parseMelody(String melodyStr, Note* notes, int &noteCount, int maxNotes) {
  noteCount = 0;
  melodyStr.trim();
  int pos = 0;
  const int MAX_TOKEN_LENGTH = 10;
  
  while (pos < melodyStr.length() && noteCount < maxNotes) {
    // Skip whitespace
    while (pos < melodyStr.length() && melodyStr[pos] == ' ') pos++;
    if (pos >= melodyStr.length()) break;
    
    // Parse note name with length limit
    String noteStr = "";
    noteStr.reserve(MAX_TOKEN_LENGTH);
    int tokenLen = 0;
    while (pos < melodyStr.length() && melodyStr[pos] != ' ' && tokenLen < MAX_TOKEN_LENGTH) {
      noteStr += melodyStr[pos];
      pos++;
      tokenLen++;
    }
    // Skip remaining chars if token too long
    while (pos < melodyStr.length() && melodyStr[pos] != ' ') pos++;
    
    if (noteStr.length() == 0) break;
    
    // Skip whitespace
    while (pos < melodyStr.length() && melodyStr[pos] == ' ') pos++;
    if (pos >= melodyStr.length()) break;
    
    // Parse duration
    char durChar = melodyStr[pos];
    pos++;
    
    // Calculate frequency
    int freq = 0;
    if (noteStr[0] == 'P' || noteStr[0] == 'p') {
      freq = 0;
    } else {
      int noteIndex = -1;
      int octave = 4;
      char noteName = noteStr[0];
      int offset = 1;
      bool isSharp = false;
      if (offset < noteStr.length() && noteStr[offset] == '#') {
        isSharp = true;
        offset++;
      }
      if (offset < noteStr.length() && noteStr[offset] >= '3' && noteStr[offset] <= '7') {
        octave = noteStr[offset] - '0';
      }
      switch (noteName) {
        case 'C': case 'c': noteIndex = 0; break;
        case 'D': case 'd': noteIndex = 2; break;
        case 'E': case 'e': noteIndex = 4; break;
        case 'F': case 'f': noteIndex = 5; break;
        case 'G': case 'g': noteIndex = 7; break;
        case 'A': case 'a': noteIndex = 9; break;
        case 'B': case 'b': noteIndex = 11; break;
      }
      if (isSharp) noteIndex++;
      if (noteIndex >= 0 && noteIndex < 12) {
        freq = noteFrequencies[noteIndex];
        if (octave > 4) {
          freq = freq << (octave - 4);
        } else if (octave < 4) {
          freq = freq >> (4 - octave);
        }
      }
    }
    
    // Calculate duration
    int duration = 250;
    switch (durChar) {
      case 'W': case 'w': duration = 1000; break;
      case 'H': case 'h': duration = 500; break;
      case 'Q': case 'q': duration = 250; break;
      case 'E': case 'e': duration = 125; break;
      case 'S': case 's': duration = 63; break;
    }
    
    // Store note with bounds check
    if (noteCount < maxNotes) {
      notes[noteCount].frequency = freq;
      notes[noteCount].duration = duration;
      noteCount++;
    }
  }
}

void playMelody(Note* notes, int noteCount) {
  if (noteCount == 0) return;
  
  buzzerNoTone(BUZZER_PIN);  // Останавливаем текущий звук
  
  currentMelody = notes;
  currentMelodyLength = noteCount;
  currentNoteIndex = 0;
  melodyPlaying = true;
  melodyInPause = false;  // Сбрасываем флаг паузы
  noteStartTime = millis();
  
  if (notes[0].frequency > 0) {
    buzzerTone(BUZZER_PIN, notes[0].frequency);
  } else {
    buzzerNoTone(BUZZER_PIN);
  }
}

void updateMelodyPlayback() {
  if (!melodyPlaying || currentMelody == nullptr || currentMelodyLength == 0) return;
  
  unsigned long now = millis();
  
  // Проверка паузы между повторами
  if (melodyInPause) {
    if (now - melodyPauseStart >= 1000) {
      // Пауза закончилась, начинаем следующее воспроизведение
      melodyInPause = false;
      currentNoteIndex = 0;
      noteStartTime = now;
      
      // Запускаем первую ноту
      if (currentMelody[0].frequency > 0) {
        buzzerTone(BUZZER_PIN, currentMelody[0].frequency);
      } else {
        buzzerNoTone(BUZZER_PIN);
      }
    }
    return;
  }
  
  // Нормальное воспроизведение - проверка окончания текущей ноты
  unsigned long elapsed = now - noteStartTime;
  if (elapsed >= currentMelody[currentNoteIndex].duration) {
    // Переходим к следующей ноте
    currentNoteIndex++;
    
    // Проверяем, закончилась ли мелодия
    if (currentNoteIndex >= currentMelodyLength) {
      // Мелодия закончилась
      
      // Для ТАЙМЕРА: останавливаем после одного воспроизведения
      if (timerTriggered) {
        stopMelody();
        timerMelodyStarted = false; // Сбрасываем флаг для таймера
        return;
      }
      
      // Для БУДИЛЬНИКА: повторяем с паузой
      buzzerNoTone(BUZZER_PIN);
      melodyInPause = true;
      melodyPauseStart = now;
      // НЕ сбрасываем alarmMelodyStarted - повтор управляется здесь через паузу
      return;
    }
    
    // Запускаем следующую ноту
    noteStartTime = now;
    if (currentMelody[currentNoteIndex].frequency > 0) {
      buzzerTone(BUZZER_PIN, currentMelody[currentNoteIndex].frequency);
    } else {
      buzzerNoTone(BUZZER_PIN);
    }
  }
}

void stopMelody() {
  melodyPlaying = false;
  melodyInPause = false;
  buzzerNoTone(BUZZER_PIN);
  currentMelody = nullptr;
  currentMelodyLength = 0;
  currentNoteIndex = 0;
}

void testMelody(String melodyStr) {
  Note testNotes[50];
  int testNoteCount = 0;
  parseMelody(melodyStr, testNotes, testNoteCount, 50);
  if (testNoteCount == 0) {
    return;
  }
  for (int i = 0; i < testNoteCount; i++) {
    if (testNotes[i].frequency > 0) {
      buzzerTone(BUZZER_PIN, testNotes[i].frequency);
    } else {
      buzzerNoTone(BUZZER_PIN);
    }
    unsigned long noteStart = millis();
    while (millis() - noteStart < testNotes[i].duration) { /* wait */ }
  }
  buzzerNoTone(BUZZER_PIN);
}
#endif

void updateLEDIndicator() {
  if (timerActive && !timerTriggered) {
    return;
  }
  if (alarmTriggered || timerTriggered) {
    return;
  }
  bool alarmOn = false;
  for (int i = 0; i < MAX_ALARMS; i++) {
    if (alarms[i].active && !alarmTriggered) {
      alarmOn = true;
      break;
    }
  }
  if (alarmOn) {
    pixel.setPixelColor(0, COLOR_ALARM);
  } else {
    pixel.setPixelColor(0, COLOR_OFF);
  }
  pixel.show();
}

void handleButton() {
  static bool btnPrev = HIGH;
  static unsigned long btnPressStart = 0;
  static bool threeSecHandled = false;
  static bool tenSecHandled = false;
  static bool setupModeTriggered = false;
  
  bool btnNow = digitalRead(BOOT_PIN);
  
  // Button just pressed
  if (btnPrev == HIGH && btnNow == LOW) {
    btnPressStart = millis();
    threeSecHandled = false;
    tenSecHandled = false;
    setupModeTriggered = false;
  }
  
  // Button held down - check for 3 sec and 10 sec
  if (btnNow == LOW) {
    unsigned long holdTime = millis() - btnPressStart;
    
    // Check for 10 second hold (Setup Mode) - priority
    if (holdTime >= 10000 && !tenSecHandled) {
      Serial.println("\r\n" + TR("[Button] ===== SETUP MODE ACTIVATED ====="));
      setupMode = true;
      setupModeTriggered = true;
      tenSecHandled = true;
      
      // Visual feedback - fast white blink
      for (int i = 0; i < 5; i++) {
        pixel.setPixelColor(0, pixel.Color(255, 255, 255));
        pixel.show();
        delay(50);
        pixel.setPixelColor(0, COLOR_OFF);
        pixel.show();
        delay(50);
      }
    }
    // Check for 3 second hold (WiFi manual enable) - only if not going to setup mode
    else if (holdTime >= 3000 && !threeSecHandled && !tenSecHandled) {
      if (wifiMode == WIFI_SMART) {
        Serial.println("\r\n" + TR("[Button] ===== LONG PRESS DETECTED ====="));
        Serial.println(TR("[Button] Hold for 10 seconds to enter setup mode"));
        threeSecHandled = true;
      }
    }
  }
  
  // Button released - execute action based on hold time
  if (btnPrev == LOW && btnNow == HIGH) {
    unsigned long holdTime = millis() - btnPressStart;
    
    // Setup mode was triggered - start it now
    if (setupModeTriggered) {
      startSetupMode();
    }
    // 3-10 second hold - WiFi manual enable
    else if (holdTime >= 3000 && holdTime < 10000 && threeSecHandled) {
      if (wifiMode == WIFI_SMART) {
        wifiManuallyEnabled = true;
        wifiManualTimeout = millis() + 10 * 60 * 1000;
        Serial.println(TR("[Button] WiFi manually enabled for 10 minutes"));
        Serial.print(TR("[Button] Current WiFi status: "));
        
        wl_status_t status = WiFi.status();
        switch(status) {
          case WL_CONNECTED: Serial.println(TR("CONNECTED")); break;
          case WL_NO_SHIELD: Serial.println(TR("NO SHIELD")); break;
          case WL_IDLE_STATUS: Serial.println(TR("IDLE")); break;
          case WL_NO_SSID_AVAIL: Serial.println(TR("NO SSID AVAILABLE")); break;
          case WL_SCAN_COMPLETED: Serial.println(TR("SCAN COMPLETED")); break;
          case WL_CONNECT_FAILED: Serial.println(TR("CONNECT FAILED")); break;
          case WL_CONNECTION_LOST: Serial.println(TR("CONNECTION LOST")); break;
          case WL_DISCONNECTED: Serial.println(TR("DISCONNECTED")); break;
          default: Serial.print(TR("UNKNOWN")); Serial.printf(" (%d)\r\n", status); break;
        }
        
        if (status == WL_CONNECTED) {
          Serial.print(TR("[Button] IP Address: "));
          Serial.println(WiFi.localIP().toString());
        } else {
          Serial.println(TR("[Button] WiFi will turn ON in next loop cycle"));
        }
        Serial.println(TR("[Button] ==============================") + String("\r\n"));
        
        // Visual feedback - green blink
        for (int i = 0; i < 3; i++) {
          pixel.setPixelColor(0, pixel.Color(0, 255, 0));
          pixel.show();
          delay(100);
          pixel.setPixelColor(0, COLOR_OFF);
          pixel.show();
          delay(100);
        }
      } else {
        Serial.println("\r\n" + TR("[Button] Long press detected, but WiFi mode is ALWAYS ON"));
        Serial.println(TR("[Button] Use 'WIFI SMART' command to enable Smart WiFi mode") + String("\r\n"));
      }
    }
    // Short press - handle alarm/timer dismiss or screen wake
    else if (holdTime < 3000 && millis() - lastBtnTime > BTN_DEBOUNCE) {
      if (alarmTriggered || timerTriggered) {
        if (alarmTriggered && triggeredAlarmIndex >= 0 && triggeredAlarmIndex < MAX_ALARMS) {
          if (!alarms[triggeredAlarmIndex].repeat) {
            alarms[triggeredAlarmIndex].active = false;
            if (alarms[triggeredAlarmIndex].saved) saveAlarmToNVS(triggeredAlarmIndex);
          }
        }
        if (timerTriggered) timerActive = false;
        
        alarmTriggered = false;
        timerTriggered = false;
        buzzerActive = false;
        triggeredAlarmIndex = -1;
        alarmTriggerTime = 0; // Сброс времени срабатывания будильника
        timerTriggerTime = 0; // Сброс времени срабатывания таймера
        
        #if BUZZER_TYPE == BUZZER_PASSIVE
        // Сброс флагов мелодий и остановка воспроизведения
        alarmMelodyStarted = false;
        timerMelodyStarted = false;
        stopMelody();
        #else
        digitalWrite(BUZZER_PIN, LOW);
        #endif
        
        currentScreen = SCREEN_CLOCK;
        infoScreenPage = 1;
        updateLEDIndicator();
        
        Serial.println("\r\n" + TR("[Button] Alarm/Timer stopped"));
      } else {
        // Check if screen is off (brightness = 0) and enable temporarily
        if (displayBrightness == 0) {
          screenTempEnabled = true;
          screenTempTimeout = millis() + 60000; // 1 minute
          setDisplayBrightness(30);
          Serial.println(TR("[Button] Screen temporarily enabled for 1 minute"));
        } else if (currentScreen == SCREEN_CLOCK) {
          currentScreen = SCREEN_INFO1;
          infoScreenPage = 1;
          infoStartTime = millis();
        } else if (currentScreen == SCREEN_INFO1) {
          currentScreen = SCREEN_INFO2;
          infoScreenPage = 2;
          infoStartTime = millis();
        } else if (currentScreen == SCREEN_INFO2) {
          currentScreen = SCREEN_INFO3;
          infoScreenPage = 3;
          infoStartTime = millis();
        } else if (currentScreen == SCREEN_INFO3) {
          currentScreen = SCREEN_INFO4;
          infoScreenPage = 4;
          infoStartTime = millis();
        } else if (currentScreen == SCREEN_INFO4) {
          currentScreen = SCREEN_CLOCK;
          infoScreenPage = 1;
        }
      }
      lastBtnTime = millis();
    }
  }
  btnPrev = btnNow;
}


void handleAutoReturn() {
  if ((currentScreen == SCREEN_INFO1 || currentScreen == SCREEN_INFO2 || 
       currentScreen == SCREEN_INFO3 || currentScreen == SCREEN_INFO4) &&
       millis() - infoStartTime > INFO_TIMEOUT) {
    currentScreen = SCREEN_CLOCK;
    infoScreenPage = 1;
  }
}

bool checkAlarmMatch() {
  if (!timeValid) return false;
  
  // Reset flag if minute has changed
  int currentMinute = timeinfo.tm_hour * 60 + timeinfo.tm_min;
  if (currentMinute != lastAlarmMinute) {
    lastAlarmMinute = currentMinute;
    alarmFiredThisMinute = false;
  }
  
  // Don't check if already fired this minute
  if (alarmFiredThisMinute) return false;
  if (alarmTriggered) return false;
  
  for (int i = 0; i < MAX_ALARMS; i++) {
    if (!alarms[i].active) continue;
    
    // Check date condition
    if (alarms[i].year > 0) {
      if (timeinfo.tm_year + 1900 != alarms[i].year || 
          timeinfo.tm_mon + 1 != alarms[i].month || 
          timeinfo.tm_mday != alarms[i].day) {
        continue;
      }
    }
    // Check weekdays condition
    else if (alarms[i].weekdays > 0) {
      int wday = timeinfo.tm_wday;
      if (wday == 0) wday = 6;
      else wday -= 1;
      if (!(alarms[i].weekdays & (1 << wday))) {
        continue;
      }
    }
    
    // Check time (hour and minute only, no second check)
    if (timeinfo.tm_hour == alarms[i].hour && 
        timeinfo.tm_min == alarms[i].minute) {
      triggeredAlarmIndex = i;
      alarmFiredThisMinute = true;
      return true;
    }
  }
  return false;
}

void addToHistory(String cmd) {
  if (cmd.length() == 0) return;
  if (historyCount > 0 && 
      commandHistory[(historyIndex - 1 + HISTORY_SIZE) % HISTORY_SIZE] == cmd) {
    return;
  }
  commandHistory[historyIndex] = cmd;
  historyIndex = (historyIndex + 1) % HISTORY_SIZE;
  if (historyCount < HISTORY_SIZE) historyCount++;
}

String getHistoryUp() {
  if (historyCount == 0) return "";
  if (historyBrowseIndex == -1) {
    tempInput = serialInput;
    historyBrowseIndex = (historyIndex - 1 + HISTORY_SIZE) % HISTORY_SIZE;
  } else {
    int prevIndex = (historyBrowseIndex - 1 + HISTORY_SIZE) % HISTORY_SIZE;
    int oldestIndex = (historyIndex - historyCount + HISTORY_SIZE) % HISTORY_SIZE;
    if (historyBrowseIndex != oldestIndex) {
      historyBrowseIndex = prevIndex;
    }
  }
  return commandHistory[historyBrowseIndex];
}

String getHistoryDown() {
  if (historyBrowseIndex == -1) return serialInput;
  int nextIndex = (historyBrowseIndex + 1) % HISTORY_SIZE;
  if (nextIndex == historyIndex) {
    historyBrowseIndex = -1;
    return tempInput;
  }
  historyBrowseIndex = nextIndex;
  return commandHistory[historyBrowseIndex];
}

void clearCurrentLine() {
  for (int i = 0; i < serialInput.length(); i++) {
    Serial.print("\b \b");
  }
}

void handleSerial() {
  if (!promptShown) {
    Serial.print("> ");
    promptShown = true;
  }
  while (Serial.available()) {
    char c = Serial.read();
    static bool escapeMode = false;
    static bool bracketMode = false;
    if (c == 27) {
      escapeMode = true;
      continue;
    }
    if (escapeMode) {
      if (c == '[') {
        bracketMode = true;
        continue;
      }
      if (bracketMode) {
        if (c == 'A') {
          String histCmd = getHistoryUp();
          if (histCmd.length() > 0) {
            clearCurrentLine();
            serialInput = histCmd;
            Serial.print(serialInput);
          }
          escapeMode = false;
          bracketMode = false;
          continue;
        } else if (c == 'B') {
          String histCmd = getHistoryDown();
          clearCurrentLine();
          serialInput = histCmd;
          Serial.print(serialInput);
          escapeMode = false;
          bracketMode = false;
          continue;
        } else if (c == 'C' || c == 'D') {
          escapeMode = false;
          bracketMode = false;
          continue;
        }
      }
      escapeMode = false;
      bracketMode = false;
      continue;
    }
    if (c == 8 || c == 127) {
      if (serialInput.length() > 0) {
        serialInput.remove(serialInput.length() - 1);
        Serial.print("\b \b");
      }
      continue;
    }
    Serial.print(c);
    if (c == '\r' || c == '\n') {
      Serial.println();
      if (serialInput.length() == 0) {
        Serial.print("> ");
        return;
      }
      String cmd = serialInput;
      addToHistory(cmd);
      historyBrowseIndex = -1;
      serialInput = "";
      cmd.trim();
      cmd.toUpperCase();
      if (cmd.equals("HELP")) {
        SerialPrintTR("=== CLOCK COMMANDS ===");
        SerialPrintTR("TIME YYYY-MM-DD HH:MM:SS - Set time manually");
        SerialPrintTR("NTP <server> - Set NTP server");
        SerialPrintTR("TZ <+/-hours> - Set timezone offset");
        SerialPrintTR("DST <+/-hours> - Set daylight saving offset");
        Serial.println("CITY <name> - Set city for weather");
        SerialPrintTR("BRIGHTNESS <0-255> - Set display brightness");
        SerialPrintTR("SAVE - Save settings to NVS");
        SerialPrintTR("RESTORE - Restore settings from NVS");
        SerialPrintTR("ERASE - Erase all NVS data");
        SerialPrintTR("STATUS - Show system status");
        SerialPrintTR("SYNC - Force NTP time sync");
        SerialPrintTR("WEATHER - Update weather data");
        SerialPrintTR("REBOOT - Restart device");
        SerialPrintTR("LANGUAGE <EN|RU> - Switch interface language");
        Serial.print("\n");
        SerialPrintTR("=== ALARM COMMANDS ===");
        SerialPrintTR("ALARM LIST - List all alarms");
        SerialPrintTR("ALARM SET <index> [YYYY-MM-DD|1234567] HH:MM [TEXT] [R] [S]");
        SerialPrintTR("ALARM CLEAR <index> - Clear alarm");
        SerialPrintTR("ALARM CLEARALL - Clear all alarms");
        SerialPrintTR("ALARM TOGGLE <index> - Toggle alarm on/off");
        Serial.println("ALARM MELODY <index> <notes> - Set alarm melody");
        Serial.print("\n"); SerialPrintTR("=== TIMER COMMANDS ===");
        SerialPrintTR("TIMER HH:MM:SS [TEXT] - Start timer");
        SerialPrintTR("TIMER MM:SS [TEXT] - Start timer");
        SerialPrintTR("TIMER SS [TEXT] - Start timer");
        SerialPrintTR("TIMER CLEAR - Clear timer");
        #if BUZZER_TYPE == BUZZER_PASSIVE
        Serial.println("\n" + TR("=== MELODY COMMANDS (Passive Buzzer) ==="));
        Serial.println(TR("MELODY ALARM <notes> - Set alarm melody"));
        Serial.println(TR("MELODY TIMER <notes> - Set timer melody"));
        Serial.println(TR("MELODY TEST <notes> - Test melody"));
        Serial.println(TR("MELODY SAVE - Save melodies to NVS"));
        #endif
        Serial.print("\n"); SerialPrintTR("=== WiFi MODE COMMANDS ===");
        SerialPrintTR("WIFI SET <ssid> <password> - Set WiFi credentials");
        SerialPrintTR("WIFI ALWAYS - Set WiFi mode to ALWAYS ON");
        SerialPrintTR("WIFI SMART - Set WiFi mode to SMART WiFi");
        SerialPrintTR("WIFI ON - Manually enable WiFi for 10 min (Smart mode)");
        SerialPrintTR("WIFI OFF - Turn WiFi off");        
        SerialPrintTR("WIFI STATUS - Show WiFi mode and status");
        Serial.print("\n"); SerialPrintTR("=== NIGHT MODE COMMANDS ===");
        SerialPrintTR("NIGHT ON <HH:MM-HH:MM> <brightness> - Enable night mode");
        SerialPrintTR("NIGHT OFF - Disable night mode");
        SerialPrintTR("NIGHT STATUS - Show night mode settings");
      }
      else if (cmd.startsWith("LANGUAGE ")) {
        String lang = cmd.substring(9);
        lang.trim();
        lang.toLowerCase();
        if (lang == "en" || lang == "ru") {
          String oldLang = currentLanguage;
          currentLanguage = lang;
          saveThemeLanguageSettings();
          Serial.print(TR("Language changed to: "));
          Serial.println(lang == "en" ? TR("English") : TR("Русский"));
          Serial.println(TR("Settings saved. Restart to see changes in startup messages."));
        } else {
          Serial.println("Usage: LANGUAGE <EN|RU>");
          Serial.println("Example: LANGUAGE RU");
        }
      }
      else if (cmd.startsWith("TIME ")) {
        setManualTime(cmd.substring(5));
        updateClockStrings();
      }
      else if (cmd.startsWith("WIFI SET ")) {
        int sp = cmd.indexOf(' ', 9);
        if (sp > 0) {
          wifiSSID = cmd.substring(9, sp);
          wifiPASS = cmd.substring(sp + 1);
          SerialPrintTR("WiFi settings updated:");
          Serial.print("  SSID: ");
          Serial.println(wifiSSID);
          Serial.print("  "); Serial.print(TR("Password:")); Serial.print(" ");
          // Show only first and last char of password
          if (wifiPASS.length() > 2) {
            Serial.print(wifiPASS.charAt(0));
            for (int i = 1; i < wifiPASS.length() - 1; i++) Serial.print("*");
            Serial.println(wifiPASS.charAt(wifiPASS.length() - 1));
          } else {
            Serial.println("***");
          }
          SerialPrintTR("Connecting to WiFi...");
          connectWiFi();
          if (WiFi.status() == WL_CONNECTED) {
            SerialPrintTR("WiFi connected successfully!");
            Serial.print(TR("IP Address: "));
            Serial.println(WiFi.localIP());
            SerialPrintTR("Use SAVE command to store settings to NVS");
          } else {
            SerialPrintTR("WiFi connection failed!");
            SerialPrintTR("Check SSID and password, then try again");
          }
        } else {
          Serial.println("Usage: WIFI SET <ssid> <password>");
        }
      }
      else if (cmd.equals("WIFI OFF")) {
        SerialPrintTR("Disabling WiFi...");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        SerialPrintTR("WiFi disabled");
      }
      else if (cmd.startsWith("NTP ")) {
        ntpServer = cmd.substring(4);
        Serial.print(TR("NTP server set to: "));
        Serial.println(ntpServer);
        if (WiFi.status() == WL_CONNECTED) {
          SerialPrintTR("Syncing time with NTP server...");
          syncTime();
          if (timeValid) {
            updateClockStrings();
            SerialPrintTR("Time synchronized successfully!");
            Serial.print(TR("Current time: "));
            Serial.printf("%02d:%02d:%02d\r\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
          } else {
            SerialPrintTR("Time sync failed!");
          }
        } else {
          SerialPrintTR("WiFi not connected - time sync skipped");
          SerialPrintTR("NTP server will be used after WiFi connects");
        }
        Serial.println("Use SAVE to store settings to NVS");
      }
      else if (cmd.startsWith("TZ ")) {
        String tzStr = cmd.substring(3);
        tzStr.trim();
        if (tzStr.length() > 0) {
          bool negative = false;
          int startIndex = 0;
          if (tzStr[0] == '+') {
            startIndex = 1;
          } else if (tzStr[0] == '-') {
            negative = true;
            startIndex = 1;
          }
          String numStr = tzStr.substring(startIndex);
          long value = numStr.toInt() * 3600;
          if (negative) value = -value;
          gmtOffset_sec = value;
          configTime(gmtOffset_sec, daylightOffset_sec, ntpServer.c_str());
          Serial.print("Timezone set to: GMT");
          Serial.print(negative ? "" : "+");
          Serial.println(gmtOffset_sec / 3600);
          Serial.println("Use SAVE to store settings to NVS");
        } else {
          Serial.println("Usage: TZ <+/-hours>");
        }
      }
      else if (cmd.startsWith("DST ")) {
        String dstStr = cmd.substring(4);
        dstStr.trim();
        if (dstStr.length() > 0) {
          bool negative = false;
          int startIndex = 0;
          if (dstStr[0] == '+') {
            startIndex = 1;
          } else if (dstStr[0] == '-') {
            negative = true;
            startIndex = 1;
          }
          String numStr = dstStr.substring(startIndex);
          long value = numStr.toInt() * 3600;
          if (negative) value = -value;
          daylightOffset_sec = value;
          configTime(gmtOffset_sec, daylightOffset_sec, ntpServer.c_str());
          Serial.print("DST offset set to: ");
          Serial.print(negative ? "" : "+");
          Serial.print(daylightOffset_sec / 3600);
          Serial.println(" hours");
          Serial.println("Use SAVE to store settings to NVS");
        } else {
          Serial.println("Usage: DST <+/-hours>");
        }
      }
      else if (cmd.startsWith("CITY ")) {
        cityName = cmd.substring(5);
        cityName.trim();
        Serial.print("Weather city set to: ");
        Serial.println(cityName);
        if (WiFi.status() == WL_CONNECTED) {
          SerialPrintTR("Updating weather...");
          updateWeather();
          Serial.print("Weather: ");
          Serial.println(weatherData);
        } else {
          SerialPrintTR("WiFi not connected - weather update skipped");
        }
        Serial.println("Use SAVE to store settings to NVS");
      }
      else if (cmd.startsWith("BRIGHTNESS ")) {
        String brightStr = cmd.substring(11);
        int bright = brightStr.toInt();
        if (bright >= 0 && bright <= 255) {
          setDisplayBrightness(bright);
          Serial.print("Display brightness set to: ");
          Serial.println(bright);
          Serial.println("Use SAVE to store settings to NVS");
        } else {
          Serial.println("Invalid brightness value!");
          Serial.println("Usage: BRIGHTNESS <0-255>");
        }
      }
      else if (cmd.equals("SAVE")) {
        Serial.println(TR("Saving settings to NVS..."));
        saveConfigToNVS();
        Serial.println(TR("  - WiFi config saved"));
        saveDisplaySettings();
        Serial.println(TR("  - Display settings saved"));
        #if BUZZER_TYPE == BUZZER_PASSIVE
        saveMelodySettings();
        Serial.println(TR("  - Melody settings saved"));
        #endif
        Serial.println(TR("All settings saved successfully!"));
      }
      else if (cmd.equals("RESTORE")) {
        Serial.println(TR("Restoring settings from NVS..."));
        loadConfigFromNVS();
        Serial.println(TR("  - WiFi config loaded"));
        loadDisplaySettings();
        setDisplayBrightness(displayBrightness);
        Serial.println(TR("  - Display settings loaded"));
        #if BUZZER_TYPE == BUZZER_PASSIVE
        loadMelodySettings();
        parseMelody(alarmMelody, alarmNotes, alarmNoteCount, 50);
        parseMelody(timerMelody, timerNotes, timerNoteCount, 50);
        Serial.println(TR("  - Melody settings loaded"));
        #endif
        Serial.print(TR("Connecting to WiFi ("));
        Serial.print(wifiSSID);
        Serial.println(")...");
        connectWiFi();
        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("WiFi connected!");
          Serial.println("Syncing time...");
          syncTime();
          if (timeValid) {
            Serial.println("Time synced successfully!");
          }
        } else {
          SerialPrintTR("WiFi connection failed!");
        }
        updateClockStrings();
        Serial.println("All settings restored!");
      }
      else if (cmd.equals("ERASE")) {
        eraseNVS();
      }
      else if (cmd.equals("STATUS")) {
        Serial.print("\r\n");
        Serial.print(TR("=== SYSTEM STATUS ==="));
        Serial.print("\r\n");
        Serial.print(TR("WiFi SSID: "));
        Serial.println(wifiSSID);
        
        Serial.print(TR("WiFi Status: "));
        wl_status_t wifiStatus = WiFi.status();
        switch(wifiStatus) {
          case WL_CONNECTED: 
            Serial.println(TR("CONNECTED")); 
            Serial.print(TR("IP Address: "));
            Serial.println(WiFi.localIP().toString());
            Serial.print(TR("Signal Strength: "));
            Serial.print(WiFi.RSSI());
            Serial.print(" ");
            Serial.println(TR("dBm"));
            break;
          case WL_NO_SHIELD: Serial.println(TR("NO SHIELD")); break;
          case WL_IDLE_STATUS: Serial.println(TR("IDLE")); break;
          case WL_NO_SSID_AVAIL: Serial.println(TR("NO SSID AVAILABLE")); break;
          case WL_SCAN_COMPLETED: Serial.println(TR("SCAN COMPLETED")); break;
          case WL_CONNECT_FAILED: Serial.println(TR("CONNECT FAILED")); break;
          case WL_CONNECTION_LOST: Serial.println(TR("CONNECTION LOST")); break;
          case WL_DISCONNECTED: Serial.println(TR("DISCONNECTED")); break;
          default: Serial.print(TR("UNKNOWN")); Serial.printf(" (%d)\r\n", wifiStatus); break;
        }
        
        Serial.print(TR("NTP Server: "));
        Serial.println(ntpServer);
        Serial.print(TR("Time Sync: "));
        Serial.println(timeValid ? TR("YES") : TR("NO"));
        Serial.print(TR("Timezone: "));
        Serial.print(TR("GMT"));
        Serial.printf("%s%d\r\n", gmtOffset_sec >= 0 ? "+" : "", gmtOffset_sec / 3600);
        Serial.print(TR("DST Offset: "));
        Serial.printf("%s%d ", daylightOffset_sec >= 0 ? "+" : "", daylightOffset_sec / 3600);
        Serial.println(TR("hours"));
        Serial.print(TR("Weather City: "));
        Serial.println(cityName);
        Serial.print(TR("Display Brightness: "));
        Serial.println(displayBrightness);
        Serial.print(TR("Night Mode: "));
        Serial.println(nightModeEnabled ? TR("ENABLED") : TR("DISABLED"));
        if (nightModeEnabled) {
          Serial.print("  ");
          Serial.print(TR("Active: "));
          Serial.println(nightModeActive ? TR("YES") : TR("NO"));
          Serial.print("  ");
          Serial.print(TR("Time: "));
          Serial.printf("%02d:%02d - %02d:%02d\r\n", 
                  nightStartHour, nightStartMinute, nightEndHour, nightEndMinute);
          Serial.print("  ");
          Serial.print(TR("Brightness: "));
          Serial.println(nightBrightness);
        }
        Serial.print(TR("WiFi Mode: "));
        Serial.println(wifiMode == WIFI_ALWAYS_ON ? TR("ALWAYS ON") : TR("SMART"));
        if (wifiMode == WIFI_SMART) {
          Serial.print("  ");
          Serial.print(TR("Auto-off timeout: "));
          Serial.print(wifiAutoOffTimeout / 60000);
          Serial.print(" ");
          Serial.println(TR("min"));
          Serial.print("  ");
          Serial.print(TR("NTP Sync interval: "));
          Serial.print(ntpSyncInterval / 60000);
          Serial.print(" ");
          Serial.println(TR("min"));
          Serial.print("  ");
          Serial.print(TR("Weather Sync interval: "));
          Serial.print(weatherSyncInterval / 60000);
          Serial.print(" ");
          Serial.println(TR("min"));
          
          unsigned long now = millis();
          if (wifiManuallyEnabled && now < wifiManualTimeout) {
            unsigned long remaining = (wifiManualTimeout - now) / 1000;
            Serial.print("  ");
            Serial.print(TR("Manual mode: "));
            Serial.print(remaining);
            Serial.print(" ");
            Serial.println(TR("sec remaining"));
          }
          
          if (wifiStatus == WL_CONNECTED) {
            unsigned long timeUntilOff = 0;
            if (now - lastWebRequest < wifiAutoOffTimeout) {
              timeUntilOff = (wifiAutoOffTimeout - (now - lastWebRequest)) / 1000;
              Serial.print("  ");
              Serial.print(TR("Time until auto-off: "));
              Serial.print(timeUntilOff);
              Serial.print(" ");
              Serial.print(TR("sec"));
              Serial.print(" (");
              Serial.print(TR("web activity"));
              Serial.println(")");
            }
          } else {
            Serial.print("  ");
            Serial.println(TR("WiFi is OFF (will wake for sync or web access)"));
            if (timeValid) {
              unsigned long timeUntilNtp = (ntpSyncInterval - (now - lastNtpSync)) / 1000;
              unsigned long timeUntilWeather = (weatherSyncInterval - (now - lastWeatherSync)) / 1000;
              if (now - lastNtpSync < ntpSyncInterval) {
                Serial.print("  ");
                Serial.print(TR("Next NTP sync in: "));
                Serial.print(timeUntilNtp);
                Serial.print(" ");
                Serial.println(TR("sec"));
              }
              if (now - lastWeatherSync < weatherSyncInterval) {
                Serial.print("  ");
                Serial.print(TR("Next weather update in: "));
                Serial.print(timeUntilWeather);
                Serial.print(" ");
                Serial.println(TR("sec"));
              }
            }
          }
        }
        #if BUZZER_TYPE == BUZZER_PASSIVE
        Serial.println(TR("Buzzer Type: PASSIVE"));
        Serial.print(TR("Alarm Melody: "));
        Serial.println(alarmMelody);
        Serial.print(TR("Timer Melody: "));
        Serial.println(timerMelody);
        #else
        Serial.println(TR("Buzzer Type: ACTIVE"));
        #endif
        if (getLocalTime(&timeinfo) && timeinfo.tm_year > 120) {
          Serial.print(TR("Current Time:"));
          Serial.printf(" %02d:%02d:%02d\r\n",
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
          Serial.print(TR("Current Date:"));
          Serial.printf(" %02d.%02d.%04d\r\n",
                  timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
        } else {
          Serial.print(TR("Current Time:"));
          Serial.println(" NOT AVAILABLE");
        }
        int activeCount = 0;
        for (int i = 0; i < MAX_ALARMS; i++) {
          if (alarms[i].active) activeCount++;
        }
        Serial.print(TR("Alarms:"));
        Serial.print(" ");
        Serial.print(activeCount);
        Serial.print(" ");
        Serial.println(TR("active (use ALARM LIST for details)"));
        if (timerActive) {
          uint64_t elapsed = esp_timer_get_time() - timerStartUs;
          uint64_t remaining = (elapsed >= timerDurationUs) ? 0 : (timerDurationUs - elapsed);
          int secRemaining = (remaining + 500000) / 1000000;
          int minRemaining = secRemaining / 60;
          secRemaining = secRemaining % 60;
          int hourRemaining = minRemaining / 60;
          minRemaining = minRemaining % 60;
          Serial.print(TR("Timer:"));
          Serial.printf(" %02d:%02d:%02d ", hourRemaining, minRemaining, secRemaining);
          Serial.print(TR("remaining"));
          if (strcmp(timerText, "TIMER") != 0) {
            Serial.printf(" '%s'", timerText);
          }
          Serial.println();
        } else {
          Serial.print(TR("Timer:"));
          Serial.print(" ");
          Serial.println(TR("OFF"));
        }
        Serial.print("AHT20: ");
        Serial.println(ahtFound ? "OK" : "NOT FOUND");
        Serial.print("BMP280: ");
        Serial.println(bmpFound ? "OK" : "NOT FOUND");
        if (ahtFound || bmpFound) {
          if (ahtFound) {
            Serial.print(TR("Temperature:"));
            Serial.printf(" %.1f C\r\n", temperature);
            Serial.print(TR("Humidity:"));
            Serial.printf(" %.1f %%\r\n", humidityVal);
          }
          if (bmpFound) {
            Serial.print(TR("Pressure:"));
            Serial.printf(" %.1f hPa\r\n", pressure);
          }
        }
        Serial.print(TR("Free RAM:"));
        Serial.printf(" %d ", esp_get_free_heap_size() / 1024);
        Serial.println(TR("KB"));
        Serial.println();
      }
      else if (cmd.equals("SYNC")) {
        if (WiFi.status() == WL_CONNECTED) {
          Serial.println(TR("Synchronizing time with NTP server..."));
          syncTime();
          if (timeValid) {
            updateClockStrings();
            SerialPrintTR("Time synchronized successfully!");
            Serial.print(TR("Current time:"));
            Serial.printf(" %02d:%02d:%02d\r\n", 
                          timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            Serial.print(TR("Current date:"));
            Serial.printf(" %02d.%02d.%04d\r\n",
                          timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
          } else {
            Serial.println(TR("Time synchronization failed!"));
            Serial.println(TR("Check NTP server and network connection"));
          }
        } else {
          Serial.println(TR("WiFi not connected!"));
          Serial.println(TR("Connect to WiFi first using: WIFI <ssid> <password>"));
        }
      }
      else if (cmd.equals("WEATHER")) {
        if (WiFi.status() == WL_CONNECTED) {
          updateWeather();
          Serial.print("Weather: ");
          Serial.println(weatherData);
        } else {
          Serial.println("WiFi not connected");
        }
      }
      else if (cmd.equals("REBOOT")) {
        Serial.println("Rebooting ESP32 in 1 second...");
        unsigned long waitStart = millis();
        while (millis() - waitStart < 1000) { /* wait */ }
        ESP.restart();
      }
      else if (cmd.startsWith("ALARM LIST")) {
        Serial.println("\n" + TR("=== ALARMS ==="));
        for (int i = 0; i < MAX_ALARMS; i++) {
          Serial.printf("[%d] ", i);
          if (alarms[i].active) {
            Serial.print(TR("ON"));
            Serial.printf(" %02d:%02d ", alarms[i].hour, alarms[i].minute);
            if (alarms[i].repeat) {
              Serial.print(TR("R"));
              Serial.print(" ");
            }
            if (alarms[i].weekdays) Serial.printf("Days:%d ", alarms[i].weekdays);
            if (alarms[i].text[0]) Serial.printf("\"%s\"", alarms[i].text);
            Serial.println();
          } else {
            Serial.println(TR("OFF"));
          }
        }
      }
      else if (cmd.startsWith("ALARM CLEAR ")) {
        int idx = cmd.substring(12).toInt();
        if (idx >= 0 && idx < MAX_ALARMS) {
          clearAlarmFromNVS(idx);
          Serial.print(TR("[Alarm] Cleared alarm"));
          Serial.print(" ");
          Serial.println(idx);
        }
      }
      else if (cmd.equals("ALARM CLEARALL")) {
        for (int i = 0; i < MAX_ALARMS; i++) {
          clearAlarmFromNVS(i);
        }
        Serial.println(TR("[Alarm] All alarms cleared"));
      }
      else if (cmd.startsWith("ALARM TOGGLE ")) {
        int idx = cmd.substring(13).toInt();
        if (idx >= 0 && idx < MAX_ALARMS) {
          alarms[idx].active = !alarms[idx].active;
          saveAlarmToNVS(idx);
          Serial.print(TR("[Alarm] Alarm"));
          Serial.print(" ");
          Serial.print(idx);
          Serial.print(" ");
          Serial.print(TR("is now"));
          Serial.print(" ");
          Serial.println(alarms[idx].active ? TR("ON") : TR("OFF"));
        }
      }
      else if (cmd.startsWith("ALARM MELODY ")) {
        int spacePos = cmd.indexOf(' ', 13);
        if (spacePos > 0) {
          int idx = cmd.substring(13, spacePos).toInt();
          if (idx >= 0 && idx < MAX_ALARMS) {
            String mel = cmd.substring(spacePos + 1);
            mel.trim();
            strncpy(alarms[idx].melody, mel.c_str(), 200);
            alarms[idx].melody[200] = '\0';
            alarms[idx].useDefaultMelody = false;
            saveAlarmToNVS(idx);
            Serial.print(TR("[Alarm] Melody set for alarm"));
            Serial.print(" ");
            Serial.println(idx);
          }
        }
      }
      else if (cmd.startsWith("ALARM SET ")) {
        int idx = -1;
        String s = cmd.substring(10);
        s.trim();
        int firstSpace = s.indexOf(' ');
        if (firstSpace > 0) {
          idx = s.substring(0, firstSpace).toInt();
          s = s.substring(firstSpace + 1);
          s.trim();
        }
        if (idx < 0 || idx >= MAX_ALARMS) {
          Serial.println(TR("[Alarm] Invalid index"));
          Serial.print("> ");
          return;
        }
        int dateType = 0;
        int year = 0, month = 0, day = 0;
        int weekdaysMask = 0;
        if (s.length() >= 10 && s.charAt(4) == '-' && s.charAt(7) == '-') {
          dateType = 1;
          year = s.substring(0, 4).toInt();
          month = s.substring(5, 7).toInt();
          day = s.substring(8, 10).toInt();
          s = s.substring(11);
          s.trim();
        }
        else if (s.length() > 0 && s[0] >= '1' && s[0] <= '7') {
          int firstColon = s.indexOf(':');
          int firstSpace = s.indexOf(' ');
          bool isWeekdays = false;
          if (firstColon == -1) {
            isWeekdays = true;
          } else if (firstSpace != -1 && firstSpace < firstColon) {
            isWeekdays = true;
          }
          if (isWeekdays) {
            dateType = 2;
            String digits = "";
            int i = 0;
            while (i < s.length() && s[i] >= '1' && s[i] <= '7') {
              digits += s[i];
              i++;
            }
            s = s.substring(i);
            s.trim();
            for (int j = 0; j < digits.length(); j++) {
              char d = digits[j];
              if (d >= '1' && d <= '7') {
                int dayNum = d - '1';
                weekdaysMask |= (1 << dayNum);
              }
            }
          }
        }
        int colonPos = s.indexOf(':');
        if (colonPos == -1 || colonPos >= 3) {
          Serial.print("> ");
          return;
        }
        int spacePos = s.indexOf(' ', colonPos + 1);
        String timePart = spacePos == -1 ? s : s.substring(0, spacePos);
        String rest = spacePos == -1 ? "" : s.substring(spacePos + 1);
        int hour = timePart.substring(0, colonPos).toInt();
        int minute = timePart.substring(colonPos + 1).toInt();
        if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
          Serial.print("> ");
          return;
        }
        if (!timeValid) {
          Serial.print("> ");
          return;
        }
        bool repeat = false;
        bool save = true;
        String textContent = "";
        rest.trim();
        while (rest.length() > 0) {
          if ((rest[0] == 'R' || rest[0] == 'r') && 
              (rest.length() == 1 || rest[1] == ' ')) {
            repeat = true;
            rest = rest.substring(1);
            rest.trim();
          } 
          else if ((rest[0] == 'S' || rest[0] == 's') && 
                   (rest.length() == 1 || rest[1] == ' ')) {
            save = true;
            rest = rest.substring(1);
            rest.trim();
          } 
          else {
            textContent = rest;
            break;
          }
        }
        char text[31] = "";
        if (textContent.length() > 0) {
          int len = min((int)textContent.length(), 30);
          textContent.substring(0, len).toCharArray(text, sizeof(text));
        }
        alarms[idx].active = true;
        alarms[idx].hour = hour;
        alarms[idx].minute = minute;
        alarms[idx].repeat = repeat;
        strncpy(alarms[idx].text, text, 30);
        alarms[idx].text[30] = '\0';
        if (dateType == 1) {
          alarms[idx].year = year;
          alarms[idx].month = month;
          alarms[idx].day = day;
          alarms[idx].weekdays = 0;
        } else if (dateType == 2) {
          alarms[idx].year = 0;
          alarms[idx].month = 0;
          alarms[idx].day = 0;
          alarms[idx].weekdays = weekdaysMask;
        } else {
          alarms[idx].year = 0;
          alarms[idx].month = 0;
          alarms[idx].day = 0;
          alarms[idx].weekdays = 0;
        }
        if (save) {
          saveAlarmToNVS(idx);
        } else {
          alarms[idx].saved = false;
          updateLEDIndicator();
        }
        Serial.print(TR("[Alarm] Set alarm"));
        Serial.print(" ");
        Serial.print(idx);
        Serial.print(" ");
        Serial.print(TR("for"));
        Serial.print(" ");
        if (dateType == 1) {
          Serial.printf("%04d-%02d-%02d ", year, month, day);
        } else if (dateType == 2) {
          Serial.print("Weekdays: ");
          if (weekdaysMask & 0x01) Serial.print("Mon ");
          if (weekdaysMask & 0x02) Serial.print("Tue ");
          if (weekdaysMask & 0x04) Serial.print("Wed ");
          if (weekdaysMask & 0x08) Serial.print("Thu ");
          if (weekdaysMask & 0x10) Serial.print("Fri ");
          if (weekdaysMask & 0x20) Serial.print("Sat ");
          if (weekdaysMask & 0x40) Serial.print("Sun ");
        } else {
          Serial.print("Daily ");
        }
        Serial.printf("%02d:%02d", hour, minute);
        if (repeat) Serial.print(" [R]");
        if (save) Serial.print(" [S]");
        if (text[0]) Serial.printf(" '%s'", text);
        Serial.println();
      }
      else if (cmd.startsWith("ALARM ")) {
        Serial.println(TR("[Alarm] Use: ALARM SET <index> ... or ALARM LIST/CLEAR/TOGGLE"));
      }
else if (cmd.equals("TIMER CLEAR")) {
  timerActive = false;
  timerTriggered = false;
  buzzerActive = false;
  #if BUZZER_TYPE == BUZZER_PASSIVE
  timerMelodyStarted = false;
  stopMelody();
  #else
  digitalWrite(BUZZER_PIN, LOW);
  #endif
}
      else if (cmd.startsWith("TIMER ")) {
        String s = cmd.substring(6);
        s.trim();
        int firstSpace = s.indexOf(' ');
        String timePart;
        String rest = "";
        if (firstSpace != -1) {
          timePart = s.substring(0, firstSpace);
          rest = s.substring(firstSpace + 1);
          rest.trim();
        } else {
          timePart = s;
        }
        int hour = 0, minute = 0, second = 0;
        int colonCount = 0;
        for (int i = 0; i < timePart.length(); i++) {
          if (timePart[i] == ':') colonCount++;
        }
        if (colonCount == 2) {
          int c1 = timePart.indexOf(':');
          int c2 = timePart.indexOf(':', c1 + 1);
          hour = timePart.substring(0, c1).toInt();
          minute = timePart.substring(c1 + 1, c2).toInt();
          second = timePart.substring(c2 + 1).toInt();
        } else if (colonCount == 1) {
          int c1 = timePart.indexOf(':');
          minute = timePart.substring(0, c1).toInt();
          second = timePart.substring(c1 + 1).toInt();
        } else if (colonCount == 0) {
          second = timePart.toInt();
        } else {
          Serial.print("> ");
          return;
        }
        if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
          Serial.print("> ");
          return;
        }
        unsigned long totalSec = (unsigned long)hour * 3600 + (unsigned long)minute * 60 + (unsigned long)second;
        if (totalSec == 0) {
          Serial.print("> ");
          return;
        }
        if (totalSec > 86400) {
          Serial.print("> ");
          return;
        }
        strcpy(timerText, "TIMER");
        if (rest.length() > 0) {
          int len = min((int)rest.length(), 30);
          rest.substring(0, len).toCharArray(timerText, sizeof(timerText));
          timerText[30] = '\0';
        }
        timerActive = true;
        timerTriggered = false;
        #if BUZZER_TYPE == BUZZER_PASSIVE
        timerMelodyStarted = false;  // Reset timer melody flag for new timer
        #endif
        timerStartUs = esp_timer_get_time();
        timerDurationUs = (uint64_t)totalSec * 1000000;
        Serial.print(TR("[Timer] Set for"));
        Serial.print(" ");
        if (hour > 0) Serial.printf("%02d:", hour);
        if (minute > 0 || hour > 0) Serial.printf("%02d:", minute);
        Serial.printf("%02d seconds", second);
        if (strcmp(timerText, "TIMER") != 0) {
          Serial.printf(" '%s'", timerText);
        }
        Serial.println();
      }
      #if BUZZER_TYPE == BUZZER_PASSIVE
      else if (cmd.startsWith("MELODY ")) {
        String subcmd = cmd.substring(7);
        subcmd.trim();
        if (subcmd.startsWith("ALARM ")) {
          alarmMelody = subcmd.substring(6);
          parseMelody(alarmMelody, alarmNotes, alarmNoteCount, 50);
        }
        else if (subcmd.startsWith("TIMER ")) {
          timerMelody = subcmd.substring(6);
          parseMelody(timerMelody, timerNotes, timerNoteCount, 50);
        }
        else if (subcmd.startsWith("TEST ")) {
          String testMel = subcmd.substring(5);
          testMelody(testMel);
        }
        else if (subcmd.equals("SAVE")) {
          saveMelodySettings();
        }
      }
      #endif
      else if (cmd.startsWith("WIFI ")) {
        String subcmd = cmd.substring(5);
        subcmd.trim();
        if (subcmd.equals("ALWAYS")) {
          wifiMode = WIFI_ALWAYS_ON;
          saveWiFiModeSettings();
          Serial.println("[WiFi] Mode set to ALWAYS ON");
          // Make sure WiFi is on
          if (WiFi.status() != WL_CONNECTED) {
            WiFi.mode(WIFI_STA);
            WiFi.begin(wifiSSID.c_str(), wifiPASS.c_str());
          }
        }
        else if (subcmd.equals("SMART")) {
          wifiMode = WIFI_SMART;
          saveWiFiModeSettings();
          Serial.println("[WiFi] Mode set to SMART WiFi");
        }
        else if (subcmd.equals("ON")) {
          if (wifiMode == WIFI_SMART) {
            wifiManuallyEnabled = true;
            wifiManualTimeout = millis() + 10 * 60 * 1000;
            Serial.println("[WiFi] Manually enabled for 10 minutes");
          } else {
            Serial.println("[WiFi] Command only works in SMART mode");
          }
        }
        else if (subcmd.equals("STATUS")) {
          Serial.print(TR("[WiFi] Mode: "));
          Serial.println(wifiMode == WIFI_ALWAYS_ON ? TR("ALWAYS ON") : TR("SMART"));
          Serial.print(TR("[WiFi] Status: "));
          Serial.println(WiFi.status() == WL_CONNECTED ? TR("CONNECTED") : TR("DISCONNECTED"));
          if (wifiManuallyEnabled) {
            unsigned long remaining = (wifiManualTimeout > millis()) ? (wifiManualTimeout - millis()) / 1000 : 0;
            Serial.printf("[WiFi] Manual mode: %lu seconds remaining\r\n", remaining);
          }
        }
      }
      else if (cmd.startsWith("NIGHT ")) {
        String subcmd = cmd.substring(6);
        subcmd.trim();
        
        if (subcmd.equals("OFF")) {
          nightModeEnabled = false;
          nightModeActive = false;
          setDisplayBrightness(displayBrightness);
          saveNightModeSettings();
          SerialPrintTR("[Night Mode] DISABLED");
          SerialPrintTR("[Night Mode] Screen brightness restored");
        }
        else if (subcmd.equals("STATUS")) {
          Serial.print(TR("[Night Mode] "));
          Serial.println(nightModeEnabled ? TR("ENABLED") : TR("DISABLED"));
          if (nightModeEnabled) {
            Serial.print(TR("[Night Mode] Time:"));
            Serial.printf(" %02d:%02d - %02d:%02d\r\n",
                    nightStartHour, nightStartMinute, nightEndHour, nightEndMinute);
            Serial.print(TR("[Night Mode] Brightness:"));
            Serial.printf(" %d\r\n", nightBrightness);
            Serial.print(TR("[Night Mode] Currently Active:"));
            Serial.printf(" %s\r\n", nightModeActive ? TR("YES") : TR("NO"));
          }
        }
        else if (subcmd.startsWith("ON ")) {
          String params = subcmd.substring(3);
          params.trim();
          
          // Parse time range: HH:MM-HH:MM
          int dashPos = params.indexOf('-');
          if (dashPos > 0) {
            String startTime = params.substring(0, dashPos);
            String rest = params.substring(dashPos + 1);
            
            // Parse brightness if present
            int spacePos = rest.indexOf(' ');
            String endTime;
            String brightnessStr;
            
            if (spacePos > 0) {
              endTime = rest.substring(0, spacePos);
              brightnessStr = rest.substring(spacePos + 1);
              brightnessStr.trim();
            } else {
              endTime = rest;
            }
            
            // Parse start time
            int colonPos = startTime.indexOf(':');
            if (colonPos > 0) {
              nightStartHour = startTime.substring(0, colonPos).toInt();
              nightStartMinute = startTime.substring(colonPos + 1).toInt();
            }
            
            // Parse end time
            colonPos = endTime.indexOf(':');
            if (colonPos > 0) {
              nightEndHour = endTime.substring(0, colonPos).toInt();
              nightEndMinute = endTime.substring(colonPos + 1).toInt();
            }
            
            // Parse brightness if provided
            if (brightnessStr.length() > 0) {
              int brightness = brightnessStr.toInt();
              if (brightness >= 0 && brightness <= 255) {
                nightBrightness = brightness;
              }
            }
            
            nightModeEnabled = true;
            saveNightModeSettings();
            
            // Apply night mode if it should be active
            checkNightMode();
            
            SerialPrintTR("[Night Mode] ENABLED");
            Serial.printf("[Night Mode] Time: %02d:%02d - %02d:%02d\r\n",
                    nightStartHour, nightStartMinute, nightEndHour, nightEndMinute);
            Serial.printf("[Night Mode] Brightness: %d\r\n", nightBrightness);
            Serial.printf("[Night Mode] Currently Active: %s\r\n", nightModeActive ? TR("YES") : TR("NO"));
          } else {
            SerialPrintTR("[Night Mode] Usage: NIGHT ON HH:MM-HH:MM [brightness]");
            SerialPrintTR("[Night Mode] Example: NIGHT ON 23:00-07:00 10");
          }
        }
        else {
          SerialPrintTR("[Night Mode] Available commands:");
          Serial.println("  NIGHT ON HH:MM-HH:MM [brightness] - Enable night mode");
          Serial.println("  NIGHT OFF - Disable night mode");
          Serial.println("  NIGHT STATUS - Show current settings");
        }
      }

      else {
        Serial.println("Unknown command.");
      }
      Serial.print("> ");
    } else {
      serialInput += c;
    }
  }
}

void loop() {
  unsigned long now = millis();
  
  // Handle Setup Mode
  if (setupModeActive) {
    dnsServer.processNextRequest();
    server.handleClient();
    handleSerial(); // Allow serial commands in setup mode
    handleSetupModeLED();
    return; // Don't process normal loop when in setup mode
  }
  
  server.handleClient();
  handleSerial();
  handleButton();
  handleAutoReturn();

  
  // Check temporary screen enable timeout
  if (screenTempEnabled && millis() > screenTempTimeout) {
    screenTempEnabled = false;
    setDisplayBrightness(0);
    Serial.println(TR("[Screen] Temporary enable timeout - screen off"));
  }
  if (now - lastBlink >= 500) {
    colonVisible = !colonVisible;
    lastBlink = now;
  }
   if (needWeatherUpdate || 
      (WiFi.status() == WL_CONNECTED && now - lastWeatherUpdate > WEATHER_UPDATE_INTERVAL)) {
    needWeatherUpdate = false;
    updateWeather();
   }
  if ((ahtFound || bmpFound) && now - lastSensorRead > SENSOR_READ_INTERVAL) {
    readSensorData();
  }
  static unsigned long lastLedBlink = 0;
  static bool ledState = false;
  bool anyAlarmActive = false;
  for (int i = 0; i < MAX_ALARMS; i++) {
    if (alarms[i].active) {
      anyAlarmActive = true;
      break;
    }
  }
  if (alarmTriggered) {
    if (now - lastLedBlink >= 250) {
      ledState = !ledState;
      pixel.setPixelColor(0, ledState ? COLOR_ALARM_TRIG : COLOR_OFF);
      pixel.show();
      lastLedBlink = now;
    }
  } else if (timerTriggered) {
    if (now - lastLedBlink >= 250) {
      ledState = !ledState;
      pixel.setPixelColor(0, ledState ? COLOR_TIMER_TRIG : COLOR_OFF);
      pixel.show();
      lastLedBlink = now;
    }
  } else if (timerActive && anyAlarmActive) {
    if (now - lastLedBlink >= 500) {
      ledState = !ledState;
      pixel.setPixelColor(0, ledState ? COLOR_TIMER : COLOR_ALARM);
      pixel.show();
      lastLedBlink = now;
    }
  } else if (timerActive) {
    if (now - lastLedBlink >= 500) {
      ledState = !ledState;
      pixel.setPixelColor(0, ledState ? COLOR_TIMER : COLOR_OFF);
      pixel.show();
      lastLedBlink = now;
    }
  } else if (anyAlarmActive) {
    pixel.setPixelColor(0, COLOR_ALARM);
    pixel.show();
  } else {
    pixel.setPixelColor(0, COLOR_OFF);
    pixel.show();
  }
  if (getLocalTime(&timeinfo) && timeinfo.tm_year > 120) {
    timeValid = true;
  }
  static unsigned long lastSec = 0;
  if (timeValid && now - lastSec >= 1000) {
    lastSec = now;
    updateClockStrings();
    
    // Check night mode every minute
    static int lastCheckMinute = -1;
    if (timeinfo.tm_min != lastCheckMinute) {
      lastCheckMinute = timeinfo.tm_min;
      checkNightMode();
    }
    
    if (checkAlarmMatch()) {
      alarmTriggered = true;
      alarmTriggerTime = now; // Сохраняем время срабатывания
      buzzerActive = true;
      currentScreen = SCREEN_ALARM;
    }
  }
  if (timerActive && !timerTriggered) {
    uint64_t elapsed = esp_timer_get_time() - timerStartUs;
    if (elapsed >= timerDurationUs) {
      timerTriggered = true;
      timerTriggerTime = now; // Сохраняем время срабатывания
      buzzerActive = true;
      currentScreen = SCREEN_TIMER;
    }
  }
  
  // Автоматическая остановка будильника/таймера через 3 минуты
  if (alarmTriggered && (now - alarmTriggerTime >= ALARM_AUTO_STOP_TIME)) {
    Serial.println(TR("[Alarm] Auto-stop after 3 minutes"));
    if (triggeredAlarmIndex >= 0 && triggeredAlarmIndex < MAX_ALARMS) {
      if (!alarms[triggeredAlarmIndex].repeat) {
        alarms[triggeredAlarmIndex].active = false;
        if (alarms[triggeredAlarmIndex].saved) saveAlarmToNVS(triggeredAlarmIndex);
      }
    }
    alarmTriggered = false;
    buzzerActive = false;
    triggeredAlarmIndex = -1;
    #if BUZZER_TYPE == BUZZER_PASSIVE
    alarmMelodyStarted = false;
    stopMelody();
    #else
    digitalWrite(BUZZER_PIN, LOW);
    #endif
    currentScreen = SCREEN_CLOCK;
    updateLEDIndicator();
  }
  
  if (timerTriggered && (now - timerTriggerTime >= ALARM_AUTO_STOP_TIME)) {
    Serial.println(TR("[Timer] Auto-stop after 3 minutes"));
    timerActive = false;
    timerTriggered = false;
    buzzerActive = false;
    #if BUZZER_TYPE == BUZZER_PASSIVE
    timerMelodyStarted = false;
    stopMelody();
    #else
    digitalWrite(BUZZER_PIN, LOW);
    #endif
    currentScreen = SCREEN_CLOCK;
    updateLEDIndicator();
  }
  
  if (buzzerActive) {
    #if BUZZER_TYPE == BUZZER_PASSIVE
    // Start melody only once per alarm/timer event
    if (alarmTriggered && !alarmMelodyStarted) {
      if (triggeredAlarmIndex >= 0 && triggeredAlarmIndex < MAX_ALARMS) {
        Alarm &a = alarms[triggeredAlarmIndex];
        if (a.useDefaultMelody) {
          playMelody(alarmNotes, alarmNoteCount);
        } else if (strlen(a.melody) > 0) {
          int customCount = 0;
          parseMelody(a.melody, customAlarmNotes, customCount, 50);
          if (customCount > 0) {
            playMelody(customAlarmNotes, customCount);
          } else {
            playMelody(alarmNotes, alarmNoteCount);
          }
        } else {
          playMelody(alarmNotes, alarmNoteCount);
        }
        alarmMelodyStarted = true;
      }
    } else if (timerTriggered && !timerMelodyStarted) {
      playMelody(timerNotes, timerNoteCount);
      timerMelodyStarted = true;
    }
    updateMelodyPlayback();
    #else
    uint64_t phase = esp_timer_get_time() % 2000000;
    bool on = false;
    if (alarmTriggered) {
      if (phase < 150000 || (phase >= 300000 && phase < 450000) || 
          (phase >= 600000 && phase < 750000)) {
        on = true;
      }
    } else if (timerTriggered) {
      if (phase < 150000 || (phase >= 300000 && phase < 450000)) {
        on = true;
      }
    }
    digitalWrite(BUZZER_PIN, on ? HIGH : LOW);
    #endif
  } else {
    #if BUZZER_TYPE == BUZZER_PASSIVE
    stopMelody();
    #else
    digitalWrite(BUZZER_PIN, LOW);
    #endif
  }
  unsigned long now_screen = millis();
  switch (currentScreen) {
    case SCREEN_INFO1:
      if (now_screen - lastScreenUpdate >= SCREEN_INFO_INTERVAL) {
        drawInfoScreen1();
        lastScreenUpdate = now_screen;
      }
      break;
    case SCREEN_INFO2:
      if (now_screen - lastScreenUpdate >= SCREEN_INFO_INTERVAL) {
        drawInfoScreen2();
        lastScreenUpdate = now_screen;
      }
      break;
    case SCREEN_INFO3:
      if (now_screen - lastScreenUpdate >= SCREEN_INFO_INTERVAL) {
        drawInfoScreen3();
        lastScreenUpdate = now_screen;
      }
      break;
    case SCREEN_INFO4:
      if (now_screen - lastScreenUpdate >= SCREEN_INFO_INTERVAL) {
        drawInfoScreen4();
        lastScreenUpdate = now_screen;
      }
      break;
    case SCREEN_ALARM:
      if (alarmTriggered && triggeredAlarmIndex >= 0 && triggeredAlarmIndex < MAX_ALARMS) {
        if (now_screen - lastScreenUpdate >= SCREEN_ALARM_INTERVAL) {
          if (strlen(alarms[triggeredAlarmIndex].text) > 0) {
            drawAlarmOrTimer(alarms[triggeredAlarmIndex].text);
          } else {
            char timeStr[10];
            sprintf(timeStr, "%02d:%02d", alarms[triggeredAlarmIndex].hour, alarms[triggeredAlarmIndex].minute);
            drawAlarmOrTimer(timeStr);
          }
          lastScreenUpdate = now_screen;
        }
      } else {
        currentScreen = SCREEN_CLOCK;
      }
      break;
    case SCREEN_TIMER:
      if (timerTriggered) {
        if (now_screen - lastScreenUpdate >= SCREEN_ALARM_INTERVAL) {
          drawAlarmOrTimer(timerText);
          lastScreenUpdate = now_screen;
        }
      } else {
        currentScreen = SCREEN_CLOCK;
      }
      break;
    case SCREEN_CLOCK:
    default:
      if (now_screen - lastScreenUpdate >= SCREEN_CLOCK_INTERVAL) {
        if (timeValid) {
          drawClock();
        } else {
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_5x7_t_cyrillic);
          int y = (BLUE_ZONE_MIN + BLUE_ZONE_MAX) / 2;
          String msg = "NO TIME SYNC";
          int w = u8g2.getStrWidth(msg.c_str());
          u8g2.setCursor((DISP_W - w) / 2, y);
          u8g2.print(msg);
          u8g2.sendBuffer();
        }
        lastScreenUpdate = now_screen;
      }
      break;
  }
  
  // Smart WiFi management
  if (wifiMode == WIFI_SMART) {
    static unsigned long lastWiFiStateChange = 0;
    static bool wifiTurningOn = false;
    const unsigned long WIFI_STATE_DEBOUNCE = 5000; // 5 seconds between state changes
    
    wl_status_t wifiStatus = WiFi.status();
    bool wifiConnected = (wifiStatus == WL_CONNECTED);
    bool wifiActive = (wifiStatus != WL_NO_SHIELD && wifiStatus != WL_IDLE_STATUS);
    bool shouldBeOn = false;
    
    // Keep WiFi on if manually enabled
    if (wifiManuallyEnabled && now < wifiManualTimeout) {
      shouldBeOn = true;
    } else {
      wifiManuallyEnabled = false;
    }
    
    // Keep WiFi on if web request was recent
    if (now - lastWebRequest < wifiAutoOffTimeout) {
      shouldBeOn = true;
    }
    
    // Wake up WiFi for periodic syncs
    if (timeValid) {
      if (now - lastNtpSync >= ntpSyncInterval) {
        shouldBeOn = true;
      }
      if (now - lastWeatherSync >= weatherSyncInterval) {
        shouldBeOn = true;
      }
    }
    
    // Turn WiFi on/off as needed (with debounce to prevent rapid changes)
    if (now - lastWiFiStateChange >= WIFI_STATE_DEBOUNCE) {
      if (shouldBeOn && !wifiActive && !wifiTurningOn) {
        // Turn WiFi ON
        Serial.println("\r\n" + TR("[Smart WiFi] ===== TURNING WiFi ON ====="));
        Serial.print(TR("[Smart WiFi] Reason:"));
        Serial.print(" ");
        if (wifiManuallyEnabled) {
          Serial.println(TR("Manual enable (button/command)"));
        } else if (now - lastWebRequest < wifiAutoOffTimeout) {
          Serial.println(TR("Recent web activity"));
        } else if (timeValid && now - lastNtpSync >= ntpSyncInterval) {
          Serial.println(TR("NTP sync needed"));
        } else if (timeValid && now - lastWeatherSync >= weatherSyncInterval) {
          Serial.println(TR("Weather update needed"));
        }
        
        WiFi.mode(WIFI_STA);
        delay(100); // Small delay for mode change
        WiFi.begin(wifiSSID.c_str(), wifiPASS.c_str());
        wifiTurningOn = true;
        lastWiFiStateChange = now;
        Serial.println(TR("[Smart WiFi] Connecting..."));
        Serial.println(TR("[Smart WiFi] ============================") + String("\r\n"));
      } else if (!shouldBeOn && wifiActive) {
        // Turn WiFi OFF
        Serial.println("\r\n" + TR("[Smart WiFi] ===== TURNING WiFi OFF ====="));
        Serial.println(TR("[Smart WiFi] No activity, powering down"));
        WiFi.disconnect(true, true); // disconnect and erase credentials from flash
        delay(100);
        WiFi.mode(WIFI_OFF);
        wifiTurningOn = false;
        lastWiFiStateChange = now;
        Serial.println(TR("[Smart WiFi] WiFi powered OFF"));
        Serial.println(TR("[Smart WiFi] =============================") + String("\r\n"));
      }
    }
    
    // Check if connection completed
    if (wifiTurningOn && wifiConnected) {
      wifiTurningOn = false;
      Serial.println("\r\n" + TR("[Smart WiFi] ===== CONNECTION SUCCESS ====="));
      Serial.print(TR("[Smart WiFi] IP Address:"));
      Serial.print(" ");
      Serial.println(WiFi.localIP().toString());
      Serial.print(TR("[Smart WiFi] Signal:"));
      Serial.print(" ");
      Serial.print(WiFi.RSSI());
      Serial.println(" " + TR("dBm"));
      Serial.println(TR("[Smart WiFi] ===========================") + String("\r\n"));
    }
    
    // Reset turning on flag if connection failed for too long
    if (wifiTurningOn && now - lastWiFiStateChange > 20000) {
      wifiTurningOn = false;
      Serial.println("\r\n" + TR("[Smart WiFi] ===== CONNECTION TIMEOUT ====="));
      Serial.println(TR("[Smart WiFi] Failed to connect in 20 sec"));
      Serial.println(TR("[Smart WiFi] Will retry on next cycle"));
      Serial.println(TR("[Smart WiFi] ===============================") + String("\r\n"));
    }
    
    // Perform periodic tasks when WiFi is connected
    if (wifiConnected) {
      if (now - lastNtpSync >= ntpSyncInterval) {
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer.c_str());
        lastNtpSync = now;
        Serial.println(TR("[Smart WiFi] NTP sync"));
      }
      if (now - lastWeatherSync >= weatherSyncInterval) {
        needWeatherUpdate = true;
        lastWeatherSync = now;
        Serial.println(TR("[Smart WiFi] Weather update scheduled"));
      }
    }
  }
}
