#ifndef CLOCK_WEBSERVER_H
#define CLOCK_WEBSERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <time.h>

// Константы из основного скетча (могут быть переопределены)
#ifndef MAX_ALARMS
#define MAX_ALARMS 10
#endif

#ifndef BUZZER_ACTIVE
#define BUZZER_ACTIVE 0
#endif

#ifndef BUZZER_PASSIVE
#define BUZZER_PASSIVE 1
#endif

#ifndef BUZZER_TYPE
#define BUZZER_TYPE BUZZER_PASSIVE
#endif

#ifndef BUZZER_PIN
#define BUZZER_PIN 12
#endif

// Alarm structure - ПОЛНОЕ определение
struct Alarm {
  bool active;
  int year;
  int month;
  int day;
  int weekdays;
  int hour;
  int minute;
  bool repeat;
  bool saved;
  char text[31];
  char melody[201];
  bool useDefaultMelody;
};

// WiFi mode enum - ПОЛНОЕ определение
enum WiFiMode {
  WIFI_ALWAYS_ON = 0,
  WIFI_SMART = 1
};

// External references to global variables
extern WebServer server;
extern struct tm timeinfo;
extern bool timeValid;
extern String wifiSSID;
extern String wifiPASS;
extern String ntpServer;
extern long gmtOffset_sec;
extern long daylightOffset_sec;
extern String cityName;
extern int displayBrightness;
extern String alarmMelody;
extern String timerMelody;
extern String currentTheme;
extern String currentLanguage;
extern char hhStr[3];
extern char mmStr[3];
extern char dateStr[11];
extern char weekdayStr[15];
extern float temperature;
extern float humidityVal;
extern float pressure;
extern String weatherData;
extern bool needWeatherUpdate;
extern unsigned long lastWebRequest;

extern Alarm alarms[];
extern int triggeredAlarmIndex;
extern bool alarmTriggered;
extern bool timerActive;
extern uint64_t timerStartUs;
extern uint64_t timerDurationUs;
extern char timerText[31];
extern bool timerTriggered;

// Night mode
extern bool nightModeEnabled;
extern bool nightModeActive;
extern int nightStartHour;
extern int nightStartMinute;
extern int nightEndHour;
extern int nightEndMinute;
extern int nightBrightness;

// WiFi mode
extern WiFiMode wifiMode;
extern unsigned long wifiAutoOffTimeout;
extern unsigned long ntpSyncInterval;
extern unsigned long weatherSyncInterval;
extern bool wifiManuallyEnabled;
extern unsigned long wifiManualTimeout;

// Additional variables
extern bool buzzerActive;

#if BUZZER_TYPE == BUZZER_PASSIVE
extern bool alarmMelodyStarted;
extern bool timerMelodyStarted;
#endif

// External functions
extern void updateClockStrings();
extern void saveAlarmToNVS(int index);
extern void clearAlarmFromNVS(int index);
extern void saveThemeLanguageSettings();
extern void saveDisplaySettings();
extern void saveMelodySettings();
extern void saveNightModeSettings();
extern void saveWiFiModeSettings();
extern void setDisplayBrightness(int brightness);
extern void saveConfigToNVS();
extern void loadConfigFromNVS();
extern void loadDisplaySettings();
extern void connectWiFi();
extern void syncTime();
extern void eraseNVS();
extern void checkNightMode();

// Note structure (always defined for melody support)
struct Note {
  int frequency;
  int duration;
};
extern Note alarmNotes[];
extern Note timerNotes[];
extern int alarmNoteCount;
extern int timerNoteCount;
extern void parseMelody(String melodyStr, Note* notes, int &noteCount, int maxNotes);
extern void buzzerTone(int pin, int frequency);
extern void buzzerNoTone(int pin);
extern void stopMelody();
extern void testMelody(String melodyStr);


// Webserver module functions
void setupWebServer();

void setManualTime(String s);
void updateWeather();
void connectWiFi();
void syncTime();
void loadConfigFromNVS();
void saveConfigToNVS();
void saveDisplaySettings();
void eraseNVS();
void clearAlarmFromNVS(int index);
void saveAlarmToNVS(int index);
void setDisplayBrightness(int brightness);
void checkNightMode();

#if BUZZER_TYPE == BUZZER_PASSIVE
void stopMelody();
void parseMelody(String melodyStr, Note* notes, int &noteCount, int maxNotes);
#endif

String getStatusJSON();
String urlDecode(String str);
String jsonEscape(String str);

#endif // CLOCK_WEBSERVER_H


