/**
 * ESP32-S3-Zero Alarm Clock
 * 
 * Advanced alarm clock and timer system for ESP32-S3-Zero board
 * Features:
 * - 0.96" OLED Display (128x64, SSD1306) with dual-color zones
 * - WiFi connectivity with web interface
 * - NTP time synchronization
 * - Configurable alarms (daily, weekdays, specific date)
 * - Countdown timer
 * - Weather display (wttr.in API)
 * - Active/Passive buzzer support with custom melodies
 * - Adjustable display brightness
 * - WS2812 RGB LED indicator
 * - Serial console interface
 * - Non-volatile storage (NVS) for settings
 * 
 * Hardware:
 * - Board: ESP32-S3-Zero (Waveshare)
 * - Display: SSD1306 OLED 128x64 (I2C: SCL=GPIO5, SDA=GPIO6)
 * - Buzzer: Active (HCM1203X) or Passive (piezo - PWM melodies) on GPIO12
 *   * Supports LOW-LEVEL trigger buzzers (like MH-FMD) - set LOW_LEVEL_TRIGGER to 1
 *   * For standard HIGH-LEVEL trigger buzzers - set LOW_LEVEL_TRIGGER to 0
 * - LED: WS2812 on GPIO21
 * - Button: BOOT button (GPIO0)
 * 
 * Display zones:
 * - Yellow zone: Y 0-20 (status/header)
 * - Blue zone: Y 21-64 (main content)
 * 
 * Author: Anton Kovalev (CheshirCa)
 * Year: 2026
 * License: CC BY-NC 4.0 (Attribution-NonCommercial)
 * GitHub: https://github.com/CheshirCa/ESP32-S3-Zero-alarm-clock
 */

#include <U8g2lib.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <time.h>
#include <Wire.h>
#include <Preferences.h>
#include <esp_timer.h>
#include <Adafruit_NeoPixel.h>
#include <driver/ledc.h> 
#include <esp32-hal-ledc.h>

// ==================== HARDWARE CONFIGURATION ====================

/**
 * Buzzer Type Selection
 * BUZZER_ACTIVE: For active buzzer with built-in oscillator (HCM1203X) - simple ON/OFF only
 * BUZZER_PASSIVE: For passive piezo buzzers - supports custom melodies via PWM
 */
#define BUZZER_ACTIVE 0
#define BUZZER_PASSIVE 1

// Select your buzzer type here
#define BUZZER_TYPE BUZZER_PASSIVE  // Change to BUZZER_ACTIVE for simple buzzer

/**
 * Low Level Trigger Support
 * Some passive buzzers (like MH-FMD) use LOW level to trigger sound
 * Set to 1 if your buzzer activates on LOW signal
 * Set to 0 for standard HIGH-triggered buzzers
 */
#define LOW_LEVEL_TRIGGER 1  // Set to 1 for MH-FMD and similar low-level trigger buzzers

// Pin definitions
#define BOOT_PIN 0        // Boot button (GPIO0)
#define BUZZER_PIN 12     // Buzzer output
#define WS2812_PIN 21     // WS2812 RGB LED
#define NUM_PIXELS 1      // Number of WS2812 LEDs

// LEDC configuration for passive buzzer (used instead of tone() for better control)
#define BUZZER_LEDC_CHANNEL 0    // LEDC channel for buzzer
#define BUZZER_LEDC_RESOLUTION 8 // 8-bit resolution (0-255)

// ==================== DISPLAY CONFIGURATION ====================

// OLED Display (72x40 visible area in 128x64 buffer)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 6, 5);

// Display dimensions and zones
const int DISP_W = 128;           // Display width
const int DISP_H = 64;            // Display height
const int X_OFF = 0;              // X offset
const int Y_OFF = 20;             // Y offset for main content
const int YELLOW_ZONE_MAX = 20;   // Yellow zone: Y 0-20
const int BLUE_ZONE_MIN = 21;     // Blue zone: Y 21-64
const int BLUE_ZONE_MAX = 64;
const int INFO_Y_OFFSET = 10;

// ==================== DEFAULT CONFIGURATION ====================

// WiFi defaults
String defSSID = "Enter_SSID";
String defPASS = "Enter_Pass";

// NTP defaults
String defNTP = "pool.ntp.org";
long defGMTOffset = 3 * 3600;      // GMT+3 (Moscow)
long defDaylightOffset = 0;

// Weather defaults
String defCity = "Moscow";          // Default city for weather

// Display defaults
const int defBrightness = 255;      // Default OLED contrast (0-255)

// Buzzer melody defaults (for passive buzzer)
// Format: Note Duration Note Duration ... (e.g., "C4 Q D4 Q E4 H")
// Notes: C C# D D# E F F# G G# A A# B P(pause)
// Octaves: 3-7
// Durations: W(whole) H(half) Q(quarter) E(eighth) S(sixteenth)
String defAlarmMelody = "E5 E P S E5 E P S E5 Q P Q E5 E P S E5 E P S E5 Q P Q E5 E P S G5 E P S C5 E P S D5 E P S E5 H P H";  // Alarm sound
String defTimerMelody = "G6 E A6 E G6 E F6 E P W";              // Timer beep pattern

// ==================== ACTIVE CONFIGURATION ====================

// WiFi settings
String wifiSSID;
String wifiPASS;

// Time settings
String ntpServer;
long gmtOffset_sec;
long daylightOffset_sec;

// Weather settings
String cityName;

// Display settings
int displayBrightness;

// Buzzer melody settings (for passive buzzer)
String alarmMelody;
String timerMelody;

// ==================== WEB SERVER ====================

WebServer server(80);

// ==================== NVS STORAGE ====================

Preferences prefs;
const char* PREF_NS = "clockcfg";           // Main config namespace
const char* ALARM_PREF_NS = "alarms";       // Alarm namespace
const char* DISPLAY_PREF_NS = "display";    // Display settings namespace
const char* MELODY_PREF_NS = "melodies";    // Melody settings namespace

// ==================== TIME MANAGEMENT ====================

struct tm timeinfo;
bool timeValid = false;

// ==================== UI STATE MANAGEMENT ====================

/**
 * Screen mode enumeration
 * Defines different display screens
 */
enum ScreenMode {
  SCREEN_CLOCK,      // Main clock display
  SCREEN_INFO1,      // Info page 1 (system status)
  SCREEN_INFO2,      // Info page 2 (network info)
  SCREEN_INFO3,      // Info page 3 (weather)
  SCREEN_ALARM,      // Alarm triggered display
  SCREEN_TIMER       // Timer triggered display
};

ScreenMode currentScreen = SCREEN_CLOCK;
int infoScreenPage = 1;

// Auto-return to clock screen after timeout
const unsigned long INFO_TIMEOUT = 10000;  // 10 seconds
unsigned long infoStartTime = 0;

// Blinking colon for clock display
bool colonVisible = true;
unsigned long lastBlink = 0;

// ==================== BUTTON HANDLING ====================

const unsigned long BTN_DEBOUNCE = 300;   // Button debounce time (ms)
unsigned long lastBtnTime = 0;

// ==================== BUZZER CONTROL ====================

bool buzzerActive = false;

#if BUZZER_TYPE == BUZZER_PASSIVE
/**
 * Musical note frequencies (Hz)
 * Array indexed by note number (0-11 for C-B)
 * Multiply by octave multiplier for different octaves
 */
const int noteFrequencies[12] = {
  262,  // C
  277,  // C#
  294,  // D
  311,  // D#
  330,  // E
  349,  // F
  370,  // F#
  392,  // G
  415,  // G#
  440,  // A
  466,  // A#
  494   // B
};

/**
 * Note duration structure for passive buzzer melodies
 */
struct Note {
  int frequency;      // Note frequency in Hz (0 for pause)
  int duration;       // Note duration in milliseconds
};

// Parsed melody storage
Note alarmNotes[50];    // Alarm melody notes
int alarmNoteCount = 0;
Note timerNotes[50];    // Timer melody notes
int timerNoteCount = 0;

// Current playback state
int currentNoteIndex = 0;
unsigned long noteStartTime = 0;
bool melodyPlaying = false;
Note* currentMelody = nullptr;
int currentMelodyLength = 0;
#endif

// ==================== WS2812 LED ====================

Adafruit_NeoPixel pixel(NUM_PIXELS, WS2812_PIN, NEO_GRB + NEO_KHZ800);

// LED color definitions
const uint32_t COLOR_OFF = pixel.Color(0, 0, 0);
const uint32_t COLOR_ALARM = pixel.Color(0, 0, 255);        // Blue for alarm set
const uint32_t COLOR_TIMER = pixel.Color(0, 255, 0);        // Green for timer active
const uint32_t COLOR_ALARM_TRIG = pixel.Color(255, 0, 0);   // Red for alarm triggered
const uint32_t COLOR_TIMER_TRIG = pixel.Color(255, 255, 0); // Yellow for timer triggered
const uint32_t COLOR_BOTH = pixel.Color(128, 0, 128);       // Purple for both active

// ==================== TIME DISPLAY STRINGS ====================

char hhStr[3] = "--";        // Hours string
char mmStr[3] = "--";        // Minutes string
char dateStr[11] = "00.00.0000";     // Date string
char weekdayStr[15] = "---";  // Weekday string

// ==================== SERIAL CONSOLE ====================

String serialInput = "";
bool promptShown = false;

// Command history for serial console
const int HISTORY_SIZE = 10;
String commandHistory[HISTORY_SIZE];
int historyIndex = 0;
int historyCount = 0;
int historyBrowseIndex = -1;
String tempInput = "";

// ==================== ALARM SYSTEM ====================

/**
 * Alarm structure
 * Supports daily, weekday-based, and date-specific alarms
 */
struct Alarm {
  bool active = false;        // Alarm enabled flag
  int year = 0;               // Specific year (0 = not date-specific)
  int month = 0;              // Specific month (0 = not date-specific)
  int day = 0;                // Specific day (0 = not date-specific)
  int weekdays = 0;           // Weekday mask (bit 0=Mon, 6=Sun, 0 = daily)
  int hour = 0;               // Alarm hour (0-23)
  int minute = 0;             // Alarm minute (0-59)
  bool repeat = false;        // Repeat after trigger
  bool saved = false;         // Saved to NVS
  char text[31] = "";         // Alarm text/label
};

Alarm myAlarm;

// ==================== TIMER SYSTEM ====================

bool timerActive = false;
uint64_t timerStartUs = 0;      // Timer start time (microseconds)
uint64_t timerDurationUs = 0;   // Timer duration (microseconds)
char timerText[31] = "";        // Timer label

// ==================== TRIGGER FLAGS ====================

bool timerTriggered = false;
bool alarmTriggered = false;

// ==================== WEEKDAY NAMES ====================

const char* weekdaysRU[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

// ==================== WEATHER DATA ====================

String weatherData = "";
unsigned long lastWeatherUpdate = 0;
const unsigned long WEATHER_UPDATE_INTERVAL = 600000;  // 10 minutes

// ==================== TEXT RENDERING ====================

/**
 * Text line structure for word-wrapped text
 */
struct TextLine {
  String text;
  int width;
};

/**
 * Font list for auto-sizing text to fit display
 * Ordered from largest to smallest
 */
const uint8_t* fontList[] = {
  u8g2_font_inr38_t_cyrillic,    // 38px
  u8g2_font_inr33_t_cyrillic,    // 33px
  u8g2_font_inr30_t_cyrillic,    // 30px
  u8g2_font_inr27_t_cyrillic,    // 27px
  u8g2_font_inr24_t_cyrillic,    // 24px
  u8g2_font_10x20_t_cyrillic,    // 20px - good large font with Cyrillic
  u8g2_font_unifont_t_cyrillic,  // 16px - universal
  u8g2_font_8x13_t_cyrillic,     // 13px
  u8g2_font_6x13_t_cyrillic,     // 13px
  u8g2_font_6x12_t_cyrillic,     // 12px
  u8g2_font_5x8_t_cyrillic,      // 8px
  u8g2_font_4x6_t_cyrillic       // 6px - smallest
};
const int fontListSize = sizeof(fontList) / sizeof(fontList[0]);

// ==================== FORWARD DECLARATIONS ====================

// Configuration management
void loadConfigFromNVS();
void saveConfigToNVS();
void loadAlarmFromNVS();
void saveAlarmToNVS();
void clearAlarmFromNVS();
void loadDisplaySettings();
void saveDisplaySettings();
void loadMelodySettings();
void saveMelodySettings();
void eraseNVS();

// Display functions
void updateClockStrings();
void drawClock();
void drawAlarmOrTimer(const char* txt);
void drawInfoScreen1();
void drawInfoScreen2();
void drawInfoScreen3();
void showSplash();
void setDisplayBrightness(int brightness);

// Network functions
void connectWiFi();
void setupWebServer();
void syncTime();
void setManualTime(String s);
void updateWeather();
String getSimpleWeather();

// UI handlers
void handleButton();
void handleAutoReturn();
void handleSerial();

// Alarm/Timer
bool checkAlarmMatch();
void updateLEDIndicator();

// Text rendering
void splitWords(String text, String* words, int &wordCount, int maxWords);
bool tryFitText(const uint8_t* font, String text, int areaWidth, int areaHeight, 
                TextLine* lines, int &lineCount, int maxLines);

// Buzzer control
#if BUZZER_TYPE == BUZZER_PASSIVE
void parseMelody(String melodyStr, Note* notes, int &noteCount, int maxNotes);
void playMelody(Note* notes, int noteCount);
void updateMelodyPlayback();
void stopMelody();
void testMelody(String melodyStr);

// Wrapper functions for buzzer control with low-level trigger support
void buzzerTone(int pin, int frequency);
void buzzerNoTone(int pin);
#endif

// ==================== BUZZER HELPER FUNCTIONS ====================

#if BUZZER_TYPE == BUZZER_PASSIVE
/**
 * Play tone on buzzer with low-level trigger support
 * Используем tone() даже для low-level триггера, но инвертируем сигнал
 */
void buzzerTone(int pin, int frequency) {
  #if LOW_LEVEL_TRIGGER
    if (frequency == 0) {
      buzzerNoTone(pin);
      return;
    }
    
    // Для low-level триггера используем tone() 
    // но будем инвертировать сигнал на аппаратном уровне
    tone(pin, frequency);
    
  #else
    // Standard high-level trigger
    tone(pin, frequency);
  #endif
}

/**
 * Stop tone on buzzer with low-level trigger support
 */
void buzzerNoTone(int pin) {
  #if LOW_LEVEL_TRIGGER
    // Для low-level триггера после noTone() устанавливаем пин HIGH
    noTone(pin);
    delayMicroseconds(100);
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    
  #else
    // Standard high-level trigger
    noTone(pin);
  #endif
}
#endif

// ==================== SETUP ====================

/**
 * Arduino setup function
 * Initializes all hardware and loads configuration
 */
void setup() {
  // Initialize GPIO
  pinMode(BOOT_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  
 // Инициализация зуммера
  #if BUZZER_TYPE == BUZZER_PASSIVE && LOW_LEVEL_TRIGGER
    // Для low-level триггера устанавливаем HIGH (без звука)
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, HIGH);
  #else
    // Для стандартного зуммера LOW (без звука)
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
  #endif

  // Initialize WS2812 LED
  pixel.begin();
  pixel.setBrightness(50);
  pixel.clear();
  pixel.show();

  // Initialize Serial console
  Serial.begin(115200);
  unsigned long start = millis();
  while (!Serial && millis() - start < 2000) delay(10);

  Serial.println("\n=== ESP32-S3-Zero Alarm Clock ===");
  Serial.println("Type HELP for available commands");
  Serial.println("Author: Anton Kovalev (CheshirCa)");
  Serial.println("GitHub: github.com/CheshirCa/ESP32-S3-Zero-alarm-clock\n");

  // Initialize OLED display
  u8g2.begin();
  u8g2.setBusClock(400000);  // 400kHz I2C
  u8g2.enableUTF8Print();
  
  // Load display settings (must be before any display operations)
  loadDisplaySettings();
  setDisplayBrightness(displayBrightness);

  // Display test message
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14_tf);
  u8g2.setCursor(40, 35);
  u8g2.print("INIT OK");
  u8g2.sendBuffer();
  delay(500);

  // Load configuration from NVS
  loadConfigFromNVS();
  loadAlarmFromNVS();
  
  #if BUZZER_TYPE == BUZZER_PASSIVE
  loadMelodySettings();
  parseMelody(alarmMelody, alarmNotes, alarmNoteCount, 50);
  parseMelody(timerMelody, timerNotes, timerNoteCount, 50);
  #endif

  // Show splash screen
  showSplash();
  
  // Initialize WiFi in station mode (even if no SSID)
  WiFi.mode(WIFI_STA);
  delay(200);  // Increased delay for stability
  
  // Initialize time system with default values (prevents crashes)
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer.c_str());
  delay(200);  // Increased delay for stability
  
  // Connect to WiFi (non-blocking)
  connectWiFi();
  
  // Sync time if WiFi connected
  if (WiFi.status() == WL_CONNECTED) {
    syncTime();
    updateWeather();
  } else {
    Serial.println("[WiFi] Not connected - time sync skipped");
    Serial.println("[WiFi] Configure with: WIFI <ssid> <password>");
  }
  
  // Update clock strings (safe now after configTime)
  updateClockStrings();
  
  // Start web server
  delay(100);  // Small delay before starting server
  setupWebServer();
  server.begin();
  delay(100);  // Small delay after starting server
  
  Serial.println("[WebServer] Started");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WebServer] Access at: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WebServer] Not accessible (WiFi not connected)");
  }
  
  Serial.println("\n=== Ready ===");
  Serial.println("Type HELP for commands\n");
}

// ==================== NVS FUNCTIONS ====================

/**
 * Load main configuration from NVS
 * Loads WiFi, NTP, and city settings
 */
void loadConfigFromNVS() {
  prefs.begin(PREF_NS, true);  // Read-only mode

  if (prefs.isKey("ssid") && !prefs.getString("ssid").isEmpty()) {
    wifiSSID = prefs.getString("ssid");
    wifiPASS = prefs.getString("pass");
    ntpServer = prefs.getString("ntp", defNTP);
    gmtOffset_sec = prefs.getLong("gmtOffset", defGMTOffset);
    daylightOffset_sec = prefs.getLong("daylightOffset", defDaylightOffset);
    cityName = prefs.getString("city", defCity);
    Serial.println("[NVS] Configuration loaded");
  } else {
    // Use defaults
    wifiSSID = defSSID;
    wifiPASS = defPASS;
    ntpServer = defNTP;
    gmtOffset_sec = defGMTOffset;
    daylightOffset_sec = defDaylightOffset;
    cityName = defCity;
    Serial.println("[NVS] Using default configuration");
  }

  prefs.end();
}

/**
 * Save main configuration to NVS
 */
void saveConfigToNVS() {
  prefs.begin(PREF_NS, false);  // Read-write mode
  prefs.putString("ssid", wifiSSID);
  prefs.putString("pass", wifiPASS);
  prefs.putString("ntp", ntpServer);
  prefs.putLong("gmtOffset", gmtOffset_sec);
  prefs.putLong("daylightOffset", daylightOffset_sec);
  prefs.putString("city", cityName);
  prefs.end();
  Serial.println("[NVS] Configuration saved");
}

/**
 * Load alarm configuration from NVS
 */
void loadAlarmFromNVS() {
  prefs.begin(ALARM_PREF_NS, true);

  if (prefs.isKey("active")) {
    myAlarm.active = prefs.getBool("active", false);
    myAlarm.year = prefs.getInt("year", 0);
    myAlarm.month = prefs.getInt("month", 0);
    myAlarm.day = prefs.getInt("day", 0);
    myAlarm.weekdays = prefs.getInt("weekdays", 0);
    myAlarm.hour = prefs.getInt("hour", 0);
    myAlarm.minute = prefs.getInt("minute", 0);
    myAlarm.repeat = prefs.getBool("repeat", false);
    myAlarm.saved = true;

    String text = prefs.getString("text", "");
    text.toCharArray(myAlarm.text, sizeof(myAlarm.text));

    Serial.println("[NVS] Alarm loaded");
  } else {
    memset(&myAlarm, 0, sizeof(myAlarm));
  }

  prefs.end();
  updateLEDIndicator();
}

/**
 * Save alarm configuration to NVS
 */
void saveAlarmToNVS() {
  prefs.begin(ALARM_PREF_NS, false);
  prefs.putBool("active", myAlarm.active);
  prefs.putInt("year", myAlarm.year);
  prefs.putInt("month", myAlarm.month);
  prefs.putInt("day", myAlarm.day);
  prefs.putInt("weekdays", myAlarm.weekdays);
  prefs.putInt("hour", myAlarm.hour);
  prefs.putInt("minute", myAlarm.minute);
  prefs.putBool("repeat", myAlarm.repeat);
  prefs.putString("text", String(myAlarm.text));
  prefs.end();

  myAlarm.saved = true;
  Serial.println("[NVS] Alarm saved");
  updateLEDIndicator();
}

/**
 * Clear alarm from NVS
 */
void clearAlarmFromNVS() {
  prefs.begin(ALARM_PREF_NS, false);
  prefs.clear();
  prefs.end();

  myAlarm.saved = false;
  Serial.println("[NVS] Alarm cleared");
  updateLEDIndicator();
}

/**
 * Load display settings from NVS
 */
void loadDisplaySettings() {
  prefs.begin(DISPLAY_PREF_NS, true);
  displayBrightness = prefs.getInt("brightness", defBrightness);
  prefs.end();
  Serial.printf("[NVS] Display brightness: %d\n", displayBrightness);
}

/**
 * Save display settings to NVS
 */
void saveDisplaySettings() {
  prefs.begin(DISPLAY_PREF_NS, false);
  prefs.putInt("brightness", displayBrightness);
  prefs.end();
  Serial.println("[NVS] Display settings saved");
}

/**
 * Load melody settings from NVS (passive buzzer only)
 */
void loadMelodySettings() {
  #if BUZZER_TYPE == BUZZER_PASSIVE
  prefs.begin(MELODY_PREF_NS, true);
  alarmMelody = prefs.getString("alarm", defAlarmMelody);
  timerMelody = prefs.getString("timer", defTimerMelody);
  prefs.end();
  Serial.println("[NVS] Melodies loaded");
  #endif
}

/**
 * Save melody settings to NVS (passive buzzer only)
 */
void saveMelodySettings() {
  #if BUZZER_TYPE == BUZZER_PASSIVE
  prefs.begin(MELODY_PREF_NS, false);
  prefs.putString("alarm", alarmMelody);
  prefs.putString("timer", timerMelody);
  prefs.end();
  Serial.println("[NVS] Melodies saved");
  #endif
}

/**
 * Erase all NVS data
 */
void eraseNVS() {
  prefs.begin(PREF_NS, false);
  prefs.clear();
  prefs.end();

  prefs.begin(ALARM_PREF_NS, false);
  prefs.clear();
  prefs.end();

  prefs.begin(DISPLAY_PREF_NS, false);
  prefs.clear();
  prefs.end();

  #if BUZZER_TYPE == BUZZER_PASSIVE
  prefs.begin(MELODY_PREF_NS, false);
  prefs.clear();
  prefs.end();
  #endif

  Serial.println("[NVS] All data erased");
}

// ==================== DISPLAY FUNCTIONS ====================

/**
 * Set display brightness/contrast
 * @param brightness Contrast value (0-255)
 */
void setDisplayBrightness(int brightness) {
  if (brightness < 0) brightness = 0;
  if (brightness > 255) brightness = 255;
  displayBrightness = brightness;
  u8g2.setContrast(brightness);
  Serial.printf("[Display] Brightness set to %d\n", brightness);
}

/**
 * Update time strings for display
 */
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

/**
 * Draw main clock screen
 */
void drawClock() {
  u8g2.clearBuffer();
  
  // Draw time in large font
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

  // Draw date and status indicators in yellow zone
  String ds = String(dateStr);
  if (myAlarm.active) ds += " A";
  if (timerActive) ds += " T";

  u8g2.setFont(u8g2_font_8x13_t_cyrillic);
  int dw = u8g2.getStrWidth(ds.c_str());
  int dx = X_OFF + (DISP_W - dw) / 2;
  u8g2.setCursor(dx, 16);
  u8g2.print(ds);

  u8g2.sendBuffer();
}

/**
 * Draw alarm or timer triggered screen
 * @param txt Custom text to display
 */
void drawAlarmOrTimer(const char* txt) {
  u8g2.clearBuffer();
  
  // Yellow zone (Y: 0-20) - "Press BOOT to stop!"
  String stopMessage = "Press BOOT!";
  u8g2.setFont(u8g2_font_6x13_t_cyrillic);
  int stopW = u8g2.getUTF8Width(stopMessage.c_str());
  int stopX = (DISP_W - stopW) / 2;
  u8g2.drawUTF8(stopX, 12, stopMessage.c_str());

  // Blue zone (Y: 21-64) - custom text with auto font sizing
  String userText = (txt && txt[0]) ? String(txt) : "ALARM";
  
  const int blueAreaY = BLUE_ZONE_MIN;
  const int blueAreaHeight = BLUE_ZONE_MAX - BLUE_ZONE_MIN;
  const int blueAreaWidth = DISP_W;
  
  // Try fonts from largest to smallest
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
    // Text too long - show error
    u8g2.setFont(u8g2_font_4x6_t_cyrillic);
    String errorMsg = "TEXT TOO LONG!";
    int errW = u8g2.getUTF8Width(errorMsg.c_str());
    int errX = (DISP_W - errW) / 2;
    int errY = blueAreaY + (blueAreaHeight / 2);
    u8g2.drawUTF8(errX, errY, errorMsg.c_str());
  } else {
    // Draw text with selected font
    u8g2.setFont(selectedFont);
    int lineHeight = u8g2.getMaxCharHeight();
    
    // Calculate total text height
    int totalHeight = lineCount * lineHeight;
    
    // Start Y position to center text vertically in blue area
    int startY = blueAreaY + (blueAreaHeight - totalHeight) / 2 + lineHeight;
    
    // Draw each line centered
    for (int i = 0; i < lineCount; i++) {
      int lineX = (DISP_W - lines[i].width) / 2;
      int lineY = startY + i * lineHeight;
      u8g2.drawUTF8(lineX, lineY, lines[i].text.c_str());
    }
  }

  u8g2.sendBuffer();
}

/**
 * Draw info screen 1 - System status
 */
void drawInfoScreen1() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_t_cyrillic);

  int y = 10;

  // Header in yellow zone
  u8g2.setCursor(28, y);
  u8g2.print("INFO 1/3");
  y += 15;

  // Info in blue zone
  u8g2.setCursor(X_OFF, y);
  u8g2.printf("Day: %s", weekdayStr);
  y += 12;

  u8g2.setCursor(X_OFF, y);
  if (myAlarm.active) {
    u8g2.printf("Alarm: %02d:%02d", myAlarm.hour, myAlarm.minute);
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

/**
 * Draw info screen 2 - Network info
 */
void drawInfoScreen2() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x13_t_cyrillic);

  int y = 10;

  // Header in yellow zone
  u8g2.setCursor(28, y);
  u8g2.print("INFO 2/3");
  y += 15;

  // Info in blue zone
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

/**
 * Draw info screen 3 - Weather
 * Ensures weather data fits in blue zone (Y: 21-64)
 */
void drawInfoScreen3() {
  u8g2.clearBuffer();
  
  // Title with same font as INFO pages
  u8g2.setFont(u8g2_font_6x13_t_cyrillic);
  u8g2.setCursor(28, 10);
  u8g2.print("WEATHER");
  
  // Check if weather data available
  if (weatherData.length() == 0 || weatherData.indexOf("No WiFi") >= 0 || 
      weatherData.indexOf("Error") >= 0) {
    u8g2.setFont(u8g2_font_5x8_t_cyrillic);
    String msg = weatherData.length() > 0 ? weatherData : "No data";
    int msgW = u8g2.getUTF8Width(msg.c_str());
    int msgX = (DISP_W - msgW) / 2;
    u8g2.drawUTF8(msgX, 40, msg.c_str());
    u8g2.sendBuffer();
    return;
  }

  // Weather text area (starting after title, around Y=22)
  const int areaY = 22;
  const int areaHeight = 42;  // 64 - 22
  const int areaWidth = DISP_W;
  
  // Extract first few lines of weather data
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
  
  // Limit total text length
  if (weatherText.length() > 100) {
    weatherText = weatherText.substring(0, 100) + "...";
  }
  
  // Try fonts from largest to smallest
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
    // Force fit with smallest font
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
  
  // Draw text in blue zone
  u8g2.setFont(selectedFont);
  int lineHeight = u8g2.getMaxCharHeight();
  
  // Calculate vertical centering
  int totalHeight = lineCount * lineHeight;
  int startY = areaY + (areaHeight - totalHeight) / 2 + lineHeight;
  
  // Ensure we don't go above blue zone
  if (startY - lineHeight < BLUE_ZONE_MIN) {
    startY = BLUE_ZONE_MIN + lineHeight;
  }
  
  // Draw each line
  for (int i = 0; i < lineCount; i++) {
    int lineX = (DISP_W - lines[i].width) / 2;
    int lineY = startY + i * lineHeight;
    
    // Ensure line stays in blue zone
    if (lineY >= BLUE_ZONE_MIN && lineY <= BLUE_ZONE_MAX) {
      u8g2.drawUTF8(lineX, lineY, lines[i].text.c_str());
    }
  }
  
  u8g2.sendBuffer();
}

/**
 * Show splash screen on startup
 */
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
  delay(1500);
}

// ==================== TEXT RENDERING FUNCTIONS ====================

/**
 * Split text into words
 * @param text Input text
 * @param words Output word array
 * @param wordCount Output word count
 * @param maxWords Maximum words
 */
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

/**
 * Try to fit text in area with given font
 * @param font Font to use
 * @param text Text to fit
 * @param areaWidth Available width
 * @param areaHeight Available height
 * @param lines Output lines array
 * @param lineCount Output line count
 * @param maxLines Maximum lines
 * @return true if text fits
 */
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

// ==================== NETWORK FUNCTIONS ====================

/**
 * URL encode a string for use in URLs
 * Handles spaces and non-ASCII characters (like Cyrillic)
 */
String urlEncode(const String& str) {
  String encoded = "";
  char c;
  char code0;
  char code1;
  
  for (unsigned int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encoded += "%20";  // Space
    } else if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;  // Safe characters
    } else {
      // Encode as %XX
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

/**
 * Connect to WiFi (non-blocking with timeout)
 */
void connectWiFi() {
  // Skip if SSID not configured
  if (wifiSSID == defSSID || wifiSSID.length() == 0) {
    Serial.println("[WiFi] No SSID configured, skipping WiFi");
    return;
  }

  Serial.println("[WiFi] Connecting...");
  Serial.println("[WiFi] SSID: " + wifiSSID);

  WiFi.disconnect(true);
  delay(100);
  WiFi.begin(wifiSSID.c_str(), wifiPASS.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > 15000) {  // 15 second timeout
      Serial.println("[WiFi] Connection timeout");
      WiFi.disconnect(true);
      return;
    }
    delay(200);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("[WiFi] Connected");
  Serial.print("[WiFi] IP: ");
  Serial.println(WiFi.localIP());
}

/**
 * Synchronize time with NTP server
 */
void syncTime() {
  timeValid = false;
  Serial.println("[NTP] Syncing time...");
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer.c_str());
  
  for (int i = 0; i < 30; i++) {
    if (getLocalTime(&timeinfo) && timeinfo.tm_year > 120) {
      timeValid = true;
      Serial.println("[NTP] Time synchronized");
      char timeStr[30];
      strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
      Serial.printf("[NTP] Current time: %s\n", timeStr);
      return;
    }
    delay(500);
  }
  
  Serial.println("[NTP] Time sync failed");
}

/**
 * Set time manually
 * @param s Time string in format "YYYY-MM-DD HH:MM:SS"
 */
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
    Serial.println("[Time] Manual time set");
  } else {
    Serial.println("[Time] Invalid format. Use: YYYY-MM-DD HH:MM:SS");
  }
}

/**
 * Get weather data from wttr.in
 * Returns formatted string: "City: Temperature\nCondition"
 */
String getSimpleWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    return "No WiFi";
  }

  HTTPClient http;
  
  // URL encode city name to handle spaces and non-ASCII characters (Cyrillic, etc.)
  String encodedCity = urlEncode(cityName);
  
  // Use simple format: %t (temperature) + space + %C (condition)
  // This format reliably returns both temperature and condition
  String url = "https://wttr.in/" + encodedCity + "?format=%t+%C&lang=en";
  
  // Clean strings for Serial output (remove any \r that might cause PuTTY display issues)
  String cleanCity = cityName;
  cleanCity.replace("\r", "");
  String cleanEncoded = encodedCity;
  cleanEncoded.replace("\r", "");
  String cleanUrl = url;
  cleanUrl.replace("\r", "");
  
  Serial.print("[Weather] City: ");
  Serial.println(cleanCity);
  Serial.print("[Weather] Encoded: ");
  Serial.println(cleanEncoded);
  Serial.print("[Weather] URL: ");
  Serial.println(cleanUrl);
  
  http.begin(url);
  http.setTimeout(5000);  // 5 second timeout
  http.setUserAgent("ESP32-Weather/1.0");  // Some servers require user agent
  
  String result = "";
  
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    payload.replace("\r", "");  // Remove carriage returns that cause PuTTY display issues
    payload.trim();
    
    // Expected format: "+2В°C Partly cloudy" or "-8В°C Snow shower"
    // Split by first space to separate temp and condition
    int spacePos = payload.indexOf(' ');
    if (spacePos > 0) {
      String temp = payload.substring(0, spacePos);
      String condition = payload.substring(spacePos + 1);
      
      // Format for display: "City: Temp\nCondition"
      result = cityName + ": " + temp + "\n" + condition;
      
      // Limit line lengths
      int newlinePos = result.indexOf('\n');
      if (newlinePos > 0) {
        String line1 = result.substring(0, newlinePos);
        String line2 = result.substring(newlinePos + 1);
        
        if (line1.length() > 18) line1 = line1.substring(0, 18);
        if (line2.length() > 18) line2 = line2.substring(0, 18);
        
        result = line1 + "\n" + line2;
      }
    } else {
      // Fallback if format unexpected
      result = cityName + ":\n" + payload;
    }
    
    // Clean result for Serial output
    String cleanResult = result;
    cleanResult.replace("\r", "");
    cleanResult.replace("\n", " / ");  // Replace newlines with separator for single-line Serial output
    Serial.print("[Weather] Success: ");
    Serial.println(cleanResult);
  } else {
    result = "Weather Error";
    Serial.printf("[Weather] HTTP error: %d\n", httpCode);
  }
  
  http.end();
  return result;
}

/**
 * Update weather data
 */
void updateWeather() {
  Serial.println("[Weather] Updating...");
  weatherData = getSimpleWeather();
  lastWeatherUpdate = millis();
  
  // Clean weatherData for Serial output
  String cleanWeather = weatherData;
  cleanWeather.replace("\r", "");
  cleanWeather.replace("\n", " / ");
  Serial.print("[Weather] Result: ");
  Serial.println(cleanWeather);
}

// ==================== BUZZER CONTROL (PASSIVE) ====================

#if BUZZER_TYPE == BUZZER_PASSIVE

/**
 * Parse melody string into Note array
 * Format: "Note Octave Duration Note Octave Duration ..."
 * Example: "C5 Q D5 Q E5 H" = C5 quarter, D5 quarter, E5 half
 * 
 * Notes: C C# D D# E F F# G G# A A# B P (P = pause)
 * Octaves: 3-7
 * Durations: W(whole=1000ms) H(half=500ms) Q(quarter=250ms) E(eighth=125ms) S(sixteenth=63ms)
 * 
 * @param melodyStr Melody string
 * @param notes Output notes array
 * @param noteCount Output note count
 * @param maxNotes Maximum notes
 */
void parseMelody(String melodyStr, Note* notes, int &noteCount, int maxNotes) {
  noteCount = 0;
  melodyStr.trim();
  
  int pos = 0;
  while (pos < melodyStr.length() && noteCount < maxNotes) {
    // Skip whitespace
    while (pos < melodyStr.length() && melodyStr[pos] == ' ') pos++;
    if (pos >= melodyStr.length()) break;
    
    // Parse note
    String noteStr = "";
    while (pos < melodyStr.length() && melodyStr[pos] != ' ') {
      noteStr += melodyStr[pos];
      pos++;
    }
    
    if (noteStr.length() == 0) break;
    
    // Parse duration
    while (pos < melodyStr.length() && melodyStr[pos] == ' ') pos++;
    if (pos >= melodyStr.length()) break;
    
    char durChar = melodyStr[pos];
    pos++;
    
    // Determine frequency
    int freq = 0;
    if (noteStr[0] == 'P' || noteStr[0] == 'p') {
      // Pause
      freq = 0;
    } else {
      // Musical note
      int noteIndex = -1;
      int octave = 4;  // Default octave
      
      // Parse note name
      char noteName = noteStr[0];
      int offset = 1;
      
      // Check for sharp
      bool isSharp = false;
      if (offset < noteStr.length() && noteStr[offset] == '#') {
        isSharp = true;
        offset++;
      }
      
      // Parse octave
      if (offset < noteStr.length() && noteStr[offset] >= '3' && noteStr[offset] <= '7') {
        octave = noteStr[offset] - '0';
      }
      
      // Get note index
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
        // Calculate frequency
        freq = noteFrequencies[noteIndex];
        // Adjust for octave (base is octave 4)
        if (octave > 4) {
          freq = freq << (octave - 4);  // Multiply by 2^(octave-4)
        } else if (octave < 4) {
          freq = freq >> (4 - octave);  // Divide by 2^(4-octave)
        }
      }
    }
    
    // Determine duration
    int duration = 250;  // Default quarter note
    switch (durChar) {
      case 'W': case 'w': duration = 1000; break;  // Whole
      case 'H': case 'h': duration = 500; break;   // Half
      case 'Q': case 'q': duration = 250; break;   // Quarter
      case 'E': case 'e': duration = 125; break;   // Eighth
      case 'S': case 's': duration = 63; break;    // Sixteenth
    }
    
    // Add note
    notes[noteCount].frequency = freq;
    notes[noteCount].duration = duration;
    noteCount++;
  }
  
  Serial.printf("[Melody] Parsed %d notes\n", noteCount);
}

/**
 * Start playing a melody
 * @param notes Melody notes array
 * @param noteCount Number of notes
 */
void playMelody(Note* notes, int noteCount) {
  if (noteCount == 0) return;
  
  currentMelody = notes;
  currentMelodyLength = noteCount;
  currentNoteIndex = 0;
  melodyPlaying = true;
  noteStartTime = millis();
  
  // Start first note
  if (notes[0].frequency > 0) {
    buzzerTone(BUZZER_PIN, notes[0].frequency);
  } else {
    buzzerNoTone(BUZZER_PIN);
  }
}

/**
 * Update melody playback (call in loop)
 */
void updateMelodyPlayback() {
  if (!melodyPlaying || currentMelody == nullptr) return;
  
  unsigned long elapsed = millis() - noteStartTime;
  
  if (elapsed >= currentMelody[currentNoteIndex].duration) {
    // Move to next note
    currentNoteIndex++;
    
    if (currentNoteIndex >= currentMelodyLength) {
      // Melody finished, restart
      currentNoteIndex = 0;
    }
    
    // Play note
    noteStartTime = millis();
    if (currentMelody[currentNoteIndex].frequency > 0) {
      buzzerTone(BUZZER_PIN, currentMelody[currentNoteIndex].frequency);
    } else {
      buzzerNoTone(BUZZER_PIN);
    }
  }
}

/**
 * Stop melody playback
 */
void stopMelody() {
  melodyPlaying = false;
  buzzerNoTone(BUZZER_PIN);
  currentMelody = nullptr;
}

/**
 * Test a melody (play once)
 * @param melodyStr Melody string to test
 */
void testMelody(String melodyStr) {
  Note testNotes[50];
  int testNoteCount = 0;
  parseMelody(melodyStr, testNotes, testNoteCount, 50);
  
  if (testNoteCount == 0) {
    Serial.println("[Melody] No notes to play");
    return;
  }
  
  Serial.println("[Melody] Testing...");
  for (int i = 0; i < testNoteCount; i++) {
    if (testNotes[i].frequency > 0) {
      buzzerTone(BUZZER_PIN, testNotes[i].frequency);
    } else {
      buzzerNoTone(BUZZER_PIN);
    }
    delay(testNotes[i].duration);
  }
  buzzerNoTone(BUZZER_PIN);
  Serial.println("[Melody] Test complete");
}

#endif

// ==================== LED INDICATOR ====================

/**
 * Update WS2812 LED indicator based on alarm/timer state
 */
void updateLEDIndicator() {
  // Don't change LED during active timer countdown
  if (timerActive && !timerTriggered) {
    return;
  }
  
  // Don't change LED during triggered states (handled in main loop)
  if (alarmTriggered || timerTriggered) {
    return;
  }
  
  // Show alarm status
  bool alarmOn = myAlarm.active && !alarmTriggered;
  
  if (alarmOn) {
    pixel.setPixelColor(0, COLOR_ALARM);
  } else {
    pixel.setPixelColor(0, COLOR_OFF);
  }
  pixel.show();
}

// ==================== UI HANDLERS ====================

/**
 * Handle BOOT button presses
 */
void handleButton() {
  static bool btnPrev = HIGH;
  bool btnNow = digitalRead(BOOT_PIN);

  if (btnPrev == HIGH && btnNow == LOW) {
    if (millis() - lastBtnTime > BTN_DEBOUNCE) {
      // Button pressed
      
      if (alarmTriggered || timerTriggered) {
        // Stop alarm/timer
        if (alarmTriggered && !myAlarm.repeat) {
          myAlarm.active = false;
          if (myAlarm.saved) clearAlarmFromNVS();
        }
        if (timerTriggered) timerActive = false;

        alarmTriggered = false;
        timerTriggered = false;
        buzzerActive = false;
        
        #if BUZZER_TYPE == BUZZER_PASSIVE
        stopMelody();
        #else
        digitalWrite(BUZZER_PIN, LOW);
        #endif
        
        currentScreen = SCREEN_CLOCK;
        infoScreenPage = 1;
        updateLEDIndicator();
        Serial.println("[Button] Signal stopped");
      } else {
        // Cycle through info screens
        if (currentScreen == SCREEN_CLOCK) {
          currentScreen = SCREEN_INFO1;
          infoScreenPage = 1;
          infoStartTime = millis();
          Serial.println("[Button] Info 1");
        } else if (currentScreen == SCREEN_INFO1) {
          currentScreen = SCREEN_INFO2;
          infoScreenPage = 2;
          infoStartTime = millis();
          Serial.println("[Button] Info 2");
        } else if (currentScreen == SCREEN_INFO2) {
          currentScreen = SCREEN_INFO3;
          infoScreenPage = 3;
          infoStartTime = millis();
          Serial.println("[Button] Info 3");
        } else if (currentScreen == SCREEN_INFO3) {
          currentScreen = SCREEN_CLOCK;
          infoScreenPage = 1;
          Serial.println("[Button] Clock");
        }
      }
      
      lastBtnTime = millis();
    }
  }
  btnPrev = btnNow;
}

/**
 * Handle auto-return to clock screen
 */
void handleAutoReturn() {
  if ((currentScreen == SCREEN_INFO1 || currentScreen == SCREEN_INFO2 || 
       currentScreen == SCREEN_INFO3) && millis() - infoStartTime > INFO_TIMEOUT) {
    currentScreen = SCREEN_CLOCK;
    infoScreenPage = 1;
  }
}

/**
 * Check if alarm time matches current time
 * @return true if alarm should trigger
 */
bool checkAlarmMatch() {
  if (!myAlarm.active || alarmTriggered) return false;

  // Check date-specific alarm
  if (myAlarm.year > 0) {
    if (timeinfo.tm_year + 1900 != myAlarm.year || 
        timeinfo.tm_mon + 1 != myAlarm.month || 
        timeinfo.tm_mday != myAlarm.day) {
      return false;
    }
  }
  // Check weekday-specific alarm
  else if (myAlarm.weekdays > 0) {
    int wday = timeinfo.tm_wday;
    if (wday == 0) wday = 6;  // Sunday = 6
    else wday -= 1;           // Monday = 0

    if (!(myAlarm.weekdays & (1 << wday))) {
      return false;
    }
  }

  // Check time match (hour, minute, and must be at second 0)
  if (timeinfo.tm_hour != myAlarm.hour || 
      timeinfo.tm_min != myAlarm.minute || 
      timeinfo.tm_sec != 0) {
    return false;
  }

  return true;
}

// ==================== COMMAND HISTORY ====================

/**
 * Add command to history
 */
void addToHistory(String cmd) {
  if (cmd.length() == 0) return;
  
  // Don't add duplicate of last command
  if (historyCount > 0 && 
      commandHistory[(historyIndex - 1 + HISTORY_SIZE) % HISTORY_SIZE] == cmd) {
    return;
  }
  
  commandHistory[historyIndex] = cmd;
  historyIndex = (historyIndex + 1) % HISTORY_SIZE;
  if (historyCount < HISTORY_SIZE) historyCount++;
}

/**
 * Get previous command from history
 */
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

/**
 * Get next command from history
 */
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

/**
 * Clear current input line
 */
void clearCurrentLine() {
  for (int i = 0; i < serialInput.length(); i++) {
    Serial.print("\b \b");
  }
}

// ==================== SERIAL CONSOLE HANDLER ====================

/**
 * Handle serial console input and commands
 */
void handleSerial() {
  if (!promptShown) {
    Serial.print("> ");
    promptShown = true;
  }

  while (Serial.available()) {
    char c = Serial.read();

    // Handle ANSI escape sequences (arrow keys)
    static bool escapeMode = false;
    static bool bracketMode = false;
    
    if (c == 27) {  // ESC
      escapeMode = true;
      continue;
    }
    
    if (escapeMode) {
      if (c == '[') {
        bracketMode = true;
        continue;
      }
      
      if (bracketMode) {
        if (c == 'A') {  // Up arrow
          String histCmd = getHistoryUp();
          if (histCmd.length() > 0) {
            clearCurrentLine();
            serialInput = histCmd;
            Serial.print(serialInput);
          }
          escapeMode = false;
          bracketMode = false;
          continue;
        } else if (c == 'B') {  // Down arrow
          String histCmd = getHistoryDown();
          clearCurrentLine();
          serialInput = histCmd;
          Serial.print(serialInput);
          escapeMode = false;
          bracketMode = false;
          continue;
        } else if (c == 'C' || c == 'D') {  // Left/Right arrow - ignore
          escapeMode = false;
          bracketMode = false;
          continue;
        }
      }
      escapeMode = false;
      bracketMode = false;
      continue;
    }

    // Handle backspace
    if (c == 8 || c == 127) {
      if (serialInput.length() > 0) {
        serialInput.remove(serialInput.length() - 1);
        Serial.print("\b \b");
      }
      continue;
    }

    Serial.print(c);

    // Handle enter
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

      // ===== HELP COMMAND =====
      if (cmd.equals("HELP")) {
        Serial.println("=== CLOCK COMMANDS ===");
        Serial.println("TIME YYYY-MM-DD HH:MM:SS - Set time manually");
        Serial.println("WIFI <ssid> <password> - Set WiFi credentials");
        Serial.println("NTP <server> - Set NTP server");
        Serial.println("TZ <+/-hours> - Set timezone offset");
        Serial.println("DST <+/-hours> - Set daylight saving offset");
        Serial.println("CITY <name> - Set city for weather");
        Serial.println("BRIGHTNESS <0-255> - Set display brightness");
        Serial.println("SAVE - Save settings to NVS");
        Serial.println("RESTORE - Restore settings from NVS");
        Serial.println("ERASE - Erase all NVS data");
        Serial.println("STATUS - Show system status");
        Serial.println("SYNC - Force NTP time sync");
        Serial.println("WEATHER - Update weather data");
        Serial.println("REBOOT - Restart device");
        Serial.println("\n=== ALARM COMMANDS ===");
        Serial.println("ALARM [YYYY-MM-DD|1234567] HH:MM [TEXT] [R] [S]");
        Serial.println("  YYYY-MM-DD = specific date");
        Serial.println("  1234567 = weekdays (1=Mon...7=Sun)");
        Serial.println("  R = repeat after trigger");
        Serial.println("  S = save to NVS");
        Serial.println("ALARM CLEAR - Clear alarm");
        Serial.println("\n=== TIMER COMMANDS ===");
        Serial.println("TIMER HH:MM:SS [TEXT] - Start timer");
        Serial.println("TIMER MM:SS [TEXT] - Start timer");
        Serial.println("TIMER SS [TEXT] - Start timer");
        Serial.println("TIMER CLEAR - Clear timer");
        #if BUZZER_TYPE == BUZZER_PASSIVE
        Serial.println("\n=== MELODY COMMANDS (Passive Buzzer) ===");
        Serial.println("MELODY ALARM <notes> - Set alarm melody");
        Serial.println("MELODY TIMER <notes> - Set timer melody");
        Serial.println("MELODY TEST <notes> - Test melody");
        Serial.println("MELODY SAVE - Save melodies to NVS");
        Serial.println("Format: C5 Q D5 Q E5 H (Note+Octave Duration)");
        Serial.println("Notes: C C# D D# E F F# G G# A A# B P(pause)");
        Serial.println("Durations: W H Q E S (whole half quarter eighth sixteenth)");
        #endif
      }
      
      // ===== TIME COMMAND =====
      else if (cmd.startsWith("TIME ")) {
        setManualTime(cmd.substring(5));
        updateClockStrings();
      }
      
      // ===== WIFI COMMAND =====
      else if (cmd.startsWith("WIFI ")) {
        int sp = cmd.indexOf(' ', 5);
        if (sp > 0) {
          wifiSSID = cmd.substring(5, sp);
          wifiPASS = cmd.substring(sp + 1);
          Serial.println("[WiFi] Credentials updated, reconnecting...");
          connectWiFi();
        } else {
          Serial.println("Usage: WIFI <ssid> <password>");
        }
      }
      
      // ===== NTP COMMAND =====
      else if (cmd.startsWith("NTP ")) {
        ntpServer = cmd.substring(4);
        Serial.println("[NTP] Server updated: " + ntpServer);
        if (WiFi.status() == WL_CONNECTED) {
          syncTime();
          updateClockStrings();
        }
      }
      
      // ===== TZ COMMAND =====
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

          Serial.printf("[TZ] GMT offset: %s%d hours\n", value >= 0 ? "+" : "", value / 3600);
          configTime(gmtOffset_sec, daylightOffset_sec, ntpServer.c_str());
        } else {
          Serial.println("Usage: TZ <+/-hours>");
        }
      }
      
      // ===== DST COMMAND =====
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

          Serial.printf("[DST] Daylight offset: %s%d hours\n", value >= 0 ? "+" : "", value / 3600);
          configTime(gmtOffset_sec, daylightOffset_sec, ntpServer.c_str());
        } else {
          Serial.println("Usage: DST <+/-hours>");
        }
      }
      
      // ===== CITY COMMAND =====
      else if (cmd.startsWith("CITY ")) {
        cityName = cmd.substring(5);
        cityName.trim();
        Serial.println("[City] Set to: " + cityName);
        if (WiFi.status() == WL_CONNECTED) {
          updateWeather();
        }
      }
      
      // ===== BRIGHTNESS COMMAND =====
      else if (cmd.startsWith("BRIGHTNESS ")) {
        String brightStr = cmd.substring(11);
        int bright = brightStr.toInt();
        setDisplayBrightness(bright);
      }
      
      // ===== SAVE COMMAND =====
      else if (cmd.equals("SAVE")) {
        saveConfigToNVS();
        saveDisplaySettings();
        #if BUZZER_TYPE == BUZZER_PASSIVE
        saveMelodySettings();
        #endif
      }
      
      // ===== RESTORE COMMAND =====
      else if (cmd.equals("RESTORE")) {
        loadConfigFromNVS();
        loadDisplaySettings();
        setDisplayBrightness(displayBrightness);
        #if BUZZER_TYPE == BUZZER_PASSIVE
        loadMelodySettings();
        parseMelody(alarmMelody, alarmNotes, alarmNoteCount, 50);
        parseMelody(timerMelody, timerNotes, timerNoteCount, 50);
        #endif
        connectWiFi();
        if (WiFi.status() == WL_CONNECTED) {
          syncTime();
        }
        updateClockStrings();
      }
      
      // ===== ERASE COMMAND =====
      else if (cmd.equals("ERASE")) {
        eraseNVS();
      }
      
      // ===== STATUS COMMAND =====
      else if (cmd.equals("STATUS")) {
        Serial.println("\n=== SYSTEM STATUS ===");
        Serial.println("WiFi SSID: " + wifiSSID);
        Serial.println("WiFi Status: " + String(WiFi.status() == WL_CONNECTED ? "CONNECTED" : "DISCONNECTED"));
        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("IP Address: " + WiFi.localIP().toString());
        }
        Serial.println("NTP Server: " + ntpServer);
        Serial.println("Time Sync: " + String(timeValid ? "YES" : "NO"));
        Serial.printf("Timezone: GMT%s%d\n", gmtOffset_sec >= 0 ? "+" : "", gmtOffset_sec / 3600);
        Serial.printf("DST Offset: %s%d hours\n", daylightOffset_sec >= 0 ? "+" : "", daylightOffset_sec / 3600);
        Serial.println("Weather City: " + cityName);
        Serial.printf("Display Brightness: %d\n", displayBrightness);
        
        #if BUZZER_TYPE == BUZZER_PASSIVE
        Serial.println("Buzzer Type: PASSIVE");
        Serial.println("Alarm Melody: " + alarmMelody);
        Serial.println("Timer Melody: " + timerMelody);
        #else
        Serial.println("Buzzer Type: ACTIVE");
        #endif

        if (getLocalTime(&timeinfo) && timeinfo.tm_year > 120) {
          char timeStr[50];
          sprintf(timeStr, "Current Time: %02d:%02d:%02d",
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
          Serial.println(timeStr);

          sprintf(timeStr, "Current Date: %02d.%02d.%04d",
                  timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
          Serial.println(timeStr);
        } else {
          Serial.println("Current Time: NOT AVAILABLE");
        }

        if (myAlarm.active) {
          Serial.print("Alarm: ");
          if (myAlarm.year > 0) {
            Serial.printf("%04d-%02d-%02d ", myAlarm.year, myAlarm.month, myAlarm.day);
          } else if (myAlarm.weekdays > 0) {
            Serial.print("Weekdays: ");
            if (myAlarm.weekdays & 0x01) Serial.print("Mon ");
            if (myAlarm.weekdays & 0x02) Serial.print("Tue ");
            if (myAlarm.weekdays & 0x04) Serial.print("Wed ");
            if (myAlarm.weekdays & 0x08) Serial.print("Thu ");
            if (myAlarm.weekdays & 0x10) Serial.print("Fri ");
            if (myAlarm.weekdays & 0x20) Serial.print("Sat ");
            if (myAlarm.weekdays & 0x40) Serial.print("Sun ");
          } else {
            Serial.print("Daily ");
          }
          Serial.printf("%02d:%02d", myAlarm.hour, myAlarm.minute);
          if (myAlarm.repeat) Serial.print(" [R]");
          if (myAlarm.saved) Serial.print(" [S]");
          if (myAlarm.text[0]) Serial.printf(" '%s'", myAlarm.text);
          Serial.println();
        } else {
          Serial.println("Alarm: OFF");
        }

        if (timerActive) {
          uint64_t elapsed = esp_timer_get_time() - timerStartUs;
          uint64_t remaining = (elapsed >= timerDurationUs) ? 0 : (timerDurationUs - elapsed);
          int secRemaining = (remaining + 500000) / 1000000;
          int minRemaining = secRemaining / 60;
          secRemaining = secRemaining % 60;
          int hourRemaining = minRemaining / 60;
          minRemaining = minRemaining % 60;

          Serial.printf("Timer: %02d:%02d:%02d remaining", hourRemaining, minRemaining, secRemaining);
          if (strcmp(timerText, "TIMER") != 0) {
            Serial.printf(" '%s'", timerText);
          }
          Serial.println();
        } else {
          Serial.println("Timer: OFF");
        }
        
        // Clean weatherData for Serial output
        String cleanWeather = weatherData.length() > 0 ? weatherData : "No data";
        cleanWeather.replace("\r", "");
        cleanWeather.replace("\n", " / ");
        Serial.print("Weather: ");
        Serial.println(cleanWeather);
        Serial.printf("Free RAM: %d KB\n", esp_get_free_heap_size() / 1024);
        Serial.println();
      }
      
      // ===== SYNC COMMAND =====
      else if (cmd.equals("SYNC")) {
        if (WiFi.status() == WL_CONNECTED) {
          syncTime();
          if (timeValid) {
            updateClockStrings();
          }
        } else {
          Serial.println("[NTP] WiFi not connected");
        }
      }
      
      // ===== WEATHER COMMAND =====
      else if (cmd.equals("WEATHER")) {
        if (WiFi.status() == WL_CONNECTED) {
          updateWeather();
        } else {
          Serial.println("[Weather] WiFi not connected");
        }
      }
      
      // ===== REBOOT COMMAND =====
      else if (cmd.equals("REBOOT")) {
        Serial.println("Rebooting...");
        delay(1000);
        ESP.restart();
      }
      
      // ===== ALARM CLEAR =====
      else if (cmd.equals("ALARM CLEAR")) {
        memset(&myAlarm, 0, sizeof(myAlarm));
        alarmTriggered = false;
        buzzerActive = false;
        
        #if BUZZER_TYPE == BUZZER_PASSIVE
        stopMelody();
        #else
        digitalWrite(BUZZER_PIN, LOW);
        #endif
        
        clearAlarmFromNVS();
        updateLEDIndicator();
        Serial.println("[Alarm] Cleared");
      }
      
      // ===== ALARM SET =====
      else if (cmd.startsWith("ALARM ")) {
        String s = cmd.substring(6);
        s.trim();

        int dateType = 0;  // 0=daily, 1=date, 2=weekdays
        int year = 0, month = 0, day = 0;
        int weekdaysMask = 0;

        // Check for date (YYYY-MM-DD)
        if (s.length() >= 10 && s.charAt(4) == '-' && s.charAt(7) == '-') {
          dateType = 1;
          year = s.substring(0, 4).toInt();
          month = s.substring(5, 7).toInt();
          day = s.substring(8, 10).toInt();
          s = s.substring(11);
          s.trim();
        }
        // Check for weekdays (1234567)
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

        // Parse time (HH:MM)
        int colonPos = s.indexOf(':');
        if (colonPos == -1 || colonPos >= 3) {
          Serial.println("Format: ALARM [YYYY-MM-DD|1234567] HH:MM [TEXT] [R] [S]");
          Serial.print("> ");
          return;
        }

        int spacePos = s.indexOf(' ', colonPos + 1);
        String timePart = spacePos == -1 ? s : s.substring(0, spacePos);
        String rest = spacePos == -1 ? "" : s.substring(spacePos + 1);

        int hour = timePart.substring(0, colonPos).toInt();
        int minute = timePart.substring(colonPos + 1).toInt();

        if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
          Serial.println("Invalid time (Hours: 0-23, Minutes: 0-59)");
          Serial.print("> ");
          return;
        }

        if (!timeValid) {
          Serial.println("Wait for time sync!");
          Serial.print("> ");
          return;
        }

        // Parse flags and text
        bool repeat = false;
        bool save = false;
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

        // Set alarm
        myAlarm.active = true;
        myAlarm.hour = hour;
        myAlarm.minute = minute;
        myAlarm.repeat = repeat;
        alarmTriggered = false;
        strcpy(myAlarm.text, text);

        if (dateType == 1) {
          myAlarm.year = year;
          myAlarm.month = month;
          myAlarm.day = day;
          myAlarm.weekdays = 0;
        } else if (dateType == 2) {
          myAlarm.year = 0;
          myAlarm.month = 0;
          myAlarm.day = 0;
          myAlarm.weekdays = weekdaysMask;
        } else {
          myAlarm.year = 0;
          myAlarm.month = 0;
          myAlarm.day = 0;
          myAlarm.weekdays = 0;
        }

        if (save) {
          saveAlarmToNVS();
        } else {
          myAlarm.saved = false;
          updateLEDIndicator();
        }

        Serial.print("[Alarm] Set for ");
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
      
      // ===== TIMER CLEAR =====
      else if (cmd.equals("TIMER CLEAR")) {
        timerActive = false;
        timerTriggered = false;
        buzzerActive = false;
        
        #if BUZZER_TYPE == BUZZER_PASSIVE
        stopMelody();
        #else
        digitalWrite(BUZZER_PIN, LOW);
        #endif
        
        Serial.println("[Timer] Cleared");
      }
      
      // ===== TIMER SET =====
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
          // HH:MM:SS
          int c1 = timePart.indexOf(':');
          int c2 = timePart.indexOf(':', c1 + 1);
          hour = timePart.substring(0, c1).toInt();
          minute = timePart.substring(c1 + 1, c2).toInt();
          second = timePart.substring(c2 + 1).toInt();
        } else if (colonCount == 1) {
          // MM:SS
          int c1 = timePart.indexOf(':');
          minute = timePart.substring(0, c1).toInt();
          second = timePart.substring(c1 + 1).toInt();
        } else if (colonCount == 0) {
          // SS
          second = timePart.toInt();
        } else {
          Serial.println("Format: TIMER [HH:]MM:SS [TEXT] or TIMER SS [TEXT]");
          Serial.print("> ");
          return;
        }

        if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
          Serial.println("Invalid time values");
          Serial.print("> ");
          return;
        }

        unsigned long totalSec = (unsigned long)hour * 3600 + (unsigned long)minute * 60 + (unsigned long)second;

        if (totalSec == 0) {
          Serial.println("Timer must be at least 1 second");
          Serial.print("> ");
          return;
        }

        if (totalSec > 86400) {
          Serial.println("Timer maximum is 24 hours");
          Serial.print("> ");
          return;
        }

        strcpy(timerText, "TIMER");

        if (rest.length() > 0) {
          int len = min((int)rest.length(), 30);
          rest.substring(0, len).toCharArray(timerText, sizeof(timerText));
        }

        timerActive = true;
        timerTriggered = false;
        timerStartUs = esp_timer_get_time();
        timerDurationUs = (uint64_t)totalSec * 1000000;

        Serial.print("[Timer] Set for ");
        if (hour > 0) Serial.printf("%02d:", hour);
        if (minute > 0 || hour > 0) Serial.printf("%02d:", minute);
        Serial.printf("%02d seconds", second);

        if (strcmp(timerText, "TIMER") != 0) {
          Serial.printf(" '%s'", timerText);
        }
        Serial.println();
      }
      
      #if BUZZER_TYPE == BUZZER_PASSIVE
      // ===== MELODY COMMANDS =====
      else if (cmd.startsWith("MELODY ")) {
        String subcmd = cmd.substring(7);
        subcmd.trim();
        
        if (subcmd.startsWith("ALARM ")) {
          alarmMelody = subcmd.substring(6);
          parseMelody(alarmMelody, alarmNotes, alarmNoteCount, 50);
          Serial.println("[Melody] Alarm melody updated");
        }
        else if (subcmd.startsWith("TIMER ")) {
          timerMelody = subcmd.substring(6);
          parseMelody(timerMelody, timerNotes, timerNoteCount, 50);
          Serial.println("[Melody] Timer melody updated");
        }
        else if (subcmd.startsWith("TEST ")) {
          String testMel = subcmd.substring(5);
          testMelody(testMel);
        }
        else if (subcmd.equals("SAVE")) {
          saveMelodySettings();
        }
        else {
          Serial.println("Usage: MELODY ALARM/TIMER/TEST <notes> or MELODY SAVE");
        }
      }
      #endif
      
      else {
        Serial.println("Unknown command. Type HELP for available commands.");
      }

      Serial.print("> ");
    } else {
      serialInput += c;
    }
  }
}

// ==================== WEB SERVER (IMPLEMENTATION CONTINUES) ====================

// I'll continue with the web server implementation in the next part...
// Due to length limitations, I'm splitting this into parts

// ==================== WEB SERVER - STATUS JSON ====================

/**
 * Get system status as JSON for web interface
 * @return JSON string with current status
 */
String getStatusJSON() {
  if (getLocalTime(&timeinfo) && timeinfo.tm_year > 120) {
    timeValid = true;
    updateClockStrings();
  }
  
  String json = "{";
  json += "\"time\":\"" + String(hhStr) + ":" + String(mmStr) + "\",";
  json += "\"date\":\"" + String(dateStr) + "\",";
  json += "\"weekday\":\"" + String(weekdayStr) + "\",";
  json += "\"ssid\":\"" + wifiSSID + "\",";
  json += "\"wifi\":\"" + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected") + "\",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"timeSync\":" + String(timeValid ? "true" : "false") + ",";
  json += "\"ntpServer\":\"" + ntpServer + "\",";
  json += "\"timezone\":" + String(gmtOffset_sec / 3600) + ",";
  json += "\"dstOffset\":" + String(daylightOffset_sec / 3600) + ",";
  json += "\"city\":\"" + cityName + "\",";
  json += "\"brightness\":" + String(displayBrightness) + ",";
  
  #if BUZZER_TYPE == BUZZER_PASSIVE
  json += "\"buzzerType\":\"passive\",";
  json += "\"alarmMelody\":\"" + alarmMelody + "\",";
  json += "\"timerMelody\":\"" + timerMelody + "\",";
  #else
  json += "\"buzzerType\":\"active\",";
  #endif
  
  if (timeValid) {
    char fullTime[20];
    sprintf(fullTime, "%04d-%02d-%02d %02d:%02d:%02d",
            timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    json += "\"fullTime\":\"" + String(fullTime) + "\",";
  } else {
    json += "\"fullTime\":\"\",";
  }
  
  // Alarm info
  json += "\"alarm\":{";
  json += "\"active\":" + String(myAlarm.active ? "true" : "false");
  if (myAlarm.active) {
    json += ",\"hour\":" + String(myAlarm.hour);
    json += ",\"minute\":" + String(myAlarm.minute);
    json += ",\"text\":\"" + String(myAlarm.text) + "\"";
    json += ",\"repeat\":" + String(myAlarm.repeat ? "true" : "false");
    json += ",\"saved\":" + String(myAlarm.saved ? "true" : "false");
    
    if (myAlarm.year > 0) {
      json += ",\"type\":\"date\"";
      json += ",\"date\":\"" + String(myAlarm.year) + "-";
      if (myAlarm.month < 10) json += "0";
      json += String(myAlarm.month) + "-";
      if (myAlarm.day < 10) json += "0";
      json += String(myAlarm.day) + "\"";
    } else if (myAlarm.weekdays > 0) {
      json += ",\"type\":\"weekdays\"";
      json += ",\"weekdays\":" + String(myAlarm.weekdays);
    } else {
      json += ",\"type\":\"daily\"";
    }
  }
  json += "},";
  
  // Timer info
  json += "\"timer\":{";
  json += "\"active\":" + String(timerActive ? "true" : "false");
  if (timerActive) {
    uint64_t elapsed = esp_timer_get_time() - timerStartUs;
    uint64_t remaining = (elapsed >= timerDurationUs) ? 0 : (timerDurationUs - elapsed);
    int secRemaining = (remaining + 500000) / 1000000;
    json += ",\"remaining\":" + String(secRemaining);
    json += ",\"text\":\"" + String(timerText) + "\"";
  }
  json += "}";
  
  json += "}";
  return json;
}

// ==================== WEB SERVER - HTML PAGE ====================

/**
 * Get main web page HTML
 * @return HTML string
 */
String getWebPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32-S3-Zero Clock</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      padding: 20px;
      color: #333;
    }
    .container {
      max-width: 900px;
      margin: 0 auto;
    }
    .card {
      background: white;
      border-radius: 12px;
      padding: 24px;
      margin-bottom: 20px;
      box-shadow: 0 4px 6px rgba(0,0,0,0.1);
    }
    h1 {
      color: white;
      text-align: center;
      margin-bottom: 20px;
      font-size: 2em;
    }
    h2 {
      color: #667eea;
      margin-bottom: 16px;
      font-size: 1.5em;
    }
    .clock-display {
      text-align: center;
      font-size: 3em;
      font-weight: bold;
      color: #667eea;
      margin: 20px 0;
    }
    .date-display {
      text-align: center;
      font-size: 1.2em;
      color: #666;
      margin-bottom: 10px;
    }
    .status-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
      gap: 12px;
      margin-top: 16px;
    }
    .status-item {
      padding: 12px;
      background: #f7f7f7;
      border-radius: 8px;
    }
    .status-label {
      font-size: 0.85em;
      color: #666;
      margin-bottom: 4px;
    }
    .status-value {
      font-size: 1.1em;
      font-weight: 600;
      color: #333;
    }
    .form-group {
      margin-bottom: 16px;
    }
    label {
      display: block;
      margin-bottom: 6px;
      font-weight: 500;
      color: #555;
    }
    input, select, textarea {
      width: 100%;
      padding: 10px;
      border: 2px solid #e0e0e0;
      border-radius: 6px;
      font-size: 1em;
      transition: border-color 0.3s;
    }
    input:focus, select:focus, textarea:focus {
      outline: none;
      border-color: #667eea;
    }
    .checkbox-group {
      display: flex;
      gap: 12px;
      flex-wrap: wrap;
    }
    .checkbox-label {
      display: flex;
      align-items: center;
      gap: 6px;
      cursor: pointer;
    }
    .checkbox-label input {
      width: auto;
    }
    button {
      background: #667eea;
      color: white;
      border: none;
      padding: 12px 24px;
      border-radius: 6px;
      font-size: 1em;
      cursor: pointer;
      transition: background 0.3s;
      width: 100%;
      font-weight: 600;
    }
    button:hover {
      background: #5568d3;
    }
    button.danger {
      background: #ef4444;
    }
    button.danger:hover {
      background: #dc2626;
    }
    button.secondary {
      background: #10b981;
    }
    button.secondary:hover {
      background: #059669;
    }
    .btn-group {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 12px;
      margin-top: 12px;
    }
    .alarm-info, .timer-info {
      background: #f0f9ff;
      border-left: 4px solid #667eea;
      padding: 12px;
      border-radius: 6px;
      margin-top: 12px;
    }
    .inactive {
      background: #f7f7f7;
      border-left-color: #ccc;
    }
    .melody-section {
      display: none;
      background: #fef3c7;
      padding: 12px;
      border-radius: 6px;
      margin-top: 12px;
    }
    .melody-help {
      font-size: 0.85em;
      color: #666;
      margin-top: 8px;
    }
    @media (max-width: 600px) {
      .clock-display { font-size: 2em; }
      h1 { font-size: 1.5em; }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32-S3-Zero Alarm Clock</h1>
    
    <!-- Clock Display -->
    <div class="card">
      <div class="clock-display" id="clock">--:--</div>
      <div class="date-display" id="date">Loading...</div>
      <div class="status-grid">
        <div class="status-item">
          <div class="status-label">WiFi SSID</div>
          <div class="status-value" id="ssid">--</div>
        </div>
        <div class="status-item">
          <div class="status-label">WiFi Status</div>
          <div class="status-value" id="wifi">--</div>
        </div>
        <div class="status-item">
          <div class="status-label">IP Address</div>
          <div class="status-value" id="ip">--</div>
        </div>
        <div class="status-item">
          <div class="status-label">Time Sync</div>
          <div class="status-value" id="sync">--</div>
        </div>
      </div>
    </div>

    <!-- System Settings -->
    <div class="card">
      <h2>System Settings</h2>
      
      <div class="form-group">
        <label>NTP Server</label>
        <input type="text" id="ntpServer" placeholder="pool.ntp.org">
      </div>
      
      <div class="form-group">
        <label>Timezone (GMT offset in hours)</label>
        <input type="number" id="timezone" min="-12" max="14" step="1" placeholder="3">
      </div>
      
      <div class="form-group">
        <label>DST Offset (hours)</label>
        <input type="number" id="dstOffset" min="-2" max="2" step="1" placeholder="0">
      </div>
      
      <div class="form-group">
        <label>Weather City</label>
        <input type="text" id="cityName" placeholder="Moscow">
      </div>
      
      <div class="form-group">
        <label>Display Brightness (0-255)</label>
        <input type="number" id="brightness" min="0" max="255" step="5" placeholder="255">
      </div>
      
      <button onclick="syncTime()">Sync Time Now</button>
      
      <div class="form-group" style="margin-top: 16px;">
        <label>Manual Time (YYYY-MM-DD HH:MM:SS)</label>
        <input type="text" id="manualTime" placeholder="2026-01-15 14:30:00">
      </div>
      
      <button onclick="setManualTime()">Set Manual Time</button>
      
      <button onclick="updateWeather()" style="margin-top: 12px;">Update Weather</button>
      
      <div class="btn-group" style="margin-top: 16px;">
        <button onclick="saveSettings()">Save Settings</button>
        <button onclick="restoreSettings()">Restore Settings</button>
      </div>
      
      <div class="btn-group">
        <button class="danger" onclick="eraseSettings()">Erase NVS</button>
        <button class="danger" onclick="rebootDevice()">Reboot</button>
      </div>
    </div>

    <!-- Alarm -->
    <div class="card">
      <h2>Alarm</h2>
      <div id="alarmStatus" class="alarm-info inactive">No alarm set</div>
      
      <div class="form-group">
        <label>Alarm Type</label>
        <select id="alarmType" onchange="updateAlarmFields()">
          <option value="daily">Daily</option>
          <option value="weekdays">Weekdays</option>
          <option value="date">Specific Date</option>
        </select>
      </div>
      
      <div id="dateField" style="display:none;" class="form-group">
        <label>Date</label>
        <input type="date" id="alarmDate">
      </div>
      
      <div id="weekdaysField" style="display:none;" class="form-group">
        <label>Select Days</label>
        <div class="checkbox-group">
          <label class="checkbox-label"><input type="checkbox" value="1"> Mon</label>
          <label class="checkbox-label"><input type="checkbox" value="2"> Tue</label>
          <label class="checkbox-label"><input type="checkbox" value="4"> Wed</label>
          <label class="checkbox-label"><input type="checkbox" value="8"> Thu</label>
          <label class="checkbox-label"><input type="checkbox" value="16"> Fri</label>
          <label class="checkbox-label"><input type="checkbox" value="32"> Sat</label>
          <label class="checkbox-label"><input type="checkbox" value="64"> Sun</label>
        </div>
      </div>
      
      <div class="form-group">
        <label>Time</label>
        <input type="time" id="alarmTime">
      </div>
      
      <div class="form-group">
        <label>Text (optional, max 30 chars)</label>
        <input type="text" id="alarmText" maxlength="30" placeholder="Wake up">
      </div>
      
      <div class="form-group">
        <label class="checkbox-label">
          <input type="checkbox" id="alarmRepeat"> Repeat after trigger
        </label>
      </div>
      
      <div class="form-group">
        <label class="checkbox-label">
          <input type="checkbox" id="alarmSave"> Save to NVS
        </label>
      </div>
      
      <div class="btn-group">
        <button onclick="setAlarm()">Set Alarm</button>
        <button class="danger" onclick="clearAlarm()">Clear Alarm</button>
      </div>
    </div>

    <!-- Timer -->
    <div class="card">
      <h2>Timer</h2>
      <div id="timerStatus" class="timer-info inactive">No timer active</div>
      
      <div class="form-group">
        <label>Duration</label>
        <input type="time" id="timerTime" step="1" value="00:05:00">
      </div>
      
      <div class="form-group">
        <label>Text (optional, max 30 chars)</label>
        <input type="text" id="timerText" maxlength="30" placeholder="Timer">
      </div>
      
      <div class="btn-group">
        <button onclick="setTimer()">Start Timer</button>
        <button class="danger" onclick="clearTimer()">Clear Timer</button>
      </div>
    </div>

    <!-- Melodies (Passive Buzzer Only) -->
    <div class="card" id="melodyCard" style="display:none;">
      <h2>Melodies (Passive Buzzer)</h2>
      
      <div class="form-group">
        <label>Alarm Melody</label>
        <textarea id="alarmMelody" rows="2" placeholder="C5 Q D5 Q E5 H" oninput="alarmMelodyLastEdit = Date.now()"></textarea>
        <div class="melody-help">
          Format: Note+Octave Duration (e.g., C5 Q = C octave 5 quarter note)<br>
          Notes: C C# D D# E F F# G G# A A# B P(pause) | Octaves: 3-7<br>
          Durations: W(whole) H(half) Q(quarter) E(eighth) S(sixteenth)
        </div>
      </div>
      
      <button class="secondary" onclick="testMelody('alarm')">Test Alarm Melody</button>
      
      <div class="form-group" style="margin-top: 16px;">
        <label>Timer Melody</label>
        <textarea id="timerMelody" rows="2" placeholder="C5 E C5 E P E" oninput="timerMelodyLastEdit = Date.now()"></textarea>
      </div>
      
      <button class="secondary" onclick="testMelody('timer')">Test Timer Melody</button>
      
      <button onclick="saveMelodies()" style="margin-top: 12px;">Save Melodies</button>
    </div>

  </div>

  <script>
    // Global buzzer type flag
    let isPassiveBuzzer = false;
    
    // Timestamps for when melody fields were last edited
    let alarmMelodyLastEdit = 0;
    let timerMelodyLastEdit = 0;
    
    // Flags to track if user is currently testing melodies
    let alarmMelodyTesting = false;
    let timerMelodyTesting = false;
    
    // Last known server values for melodies
    let alarmMelodyLastServerValue = '';
    let timerMelodyLastServerValue = '';
    
    // Flag to track if melody fields have been initialized
    let melodyFieldsInitialized = false;

    function updateStatus() {
      fetch('/status')
        .then(r => r.json())
        .then(data => {
          // Update display
          document.getElementById('clock').textContent = data.time;
          document.getElementById('date').textContent = data.date + ' - ' + data.weekday;
          document.getElementById('ssid').textContent = data.ssid;
          document.getElementById('wifi').textContent = data.wifi;
          document.getElementById('ip').textContent = data.ip;
          document.getElementById('sync').textContent = data.timeSync ? 'Synced' : 'Not synced';
          
          // Update settings
          document.getElementById('ntpServer').value = data.ntpServer;
          document.getElementById('timezone').value = data.timezone;
          document.getElementById('dstOffset').value = data.dstOffset;
          document.getElementById('cityName').value = data.city;
          document.getElementById('brightness').value = data.brightness;
          
          if (data.fullTime) {
            document.getElementById('manualTime').value = data.fullTime;
          }
          
          // Check buzzer type and show melody card if passive
          isPassiveBuzzer = (data.buzzerType === 'passive');
          if (isPassiveBuzzer) {
            document.getElementById('melodyCard').style.display = 'block';
            
            const alarmMelodyField = document.getElementById('alarmMelody');
            const timerMelodyField = document.getElementById('timerMelody');
            
            // Initialize melody fields on first load
            if (!melodyFieldsInitialized) {
              melodyFieldsInitialized = true;
              alarmMelodyLastServerValue = data.alarmMelody || '';
              timerMelodyLastServerValue = data.timerMelody || '';
              
              // Try to restore drafts from localStorage
              const savedAlarmMelody = localStorage.getItem('alarmMelodyDraft');
              const savedTimerMelody = localStorage.getItem('timerMelodyDraft');
              
              if (savedAlarmMelody !== null) {
                alarmMelodyField.value = savedAlarmMelody;
              } else {
                alarmMelodyField.value = data.alarmMelody || '';
              }
              
              if (savedTimerMelody !== null) {
                timerMelodyField.value = savedTimerMelody;
              } else {
                timerMelodyField.value = data.timerMelody || '';
              }
            } else {
              // On subsequent updates, only update if:
              // 1. User hasn't changed the field from last server value
              // 2. Field is not currently focused
              // 3. Not currently testing this melody
              
              const currentAlarmValue = alarmMelodyField.value;
              const currentTimerValue = timerMelodyField.value;
              
              const alarmChangedByUser = currentAlarmValue !== alarmMelodyLastServerValue;
              const timerChangedByUser = currentTimerValue !== timerMelodyLastServerValue;
              
              // Update server values
              alarmMelodyLastServerValue = data.alarmMelody || '';
              timerMelodyLastServerValue = data.timerMelody || '';
              
              // Update alarm melody field only if:
              // - User hasn't changed it from previous server value
              // - OR field is not focused and not being tested
              if (!alarmMelodyTesting && 
                  (!alarmChangedByUser || (document.activeElement !== alarmMelodyField))) {
                // Check localStorage for drafts
                const savedAlarmMelody = localStorage.getItem('alarmMelodyDraft');
                if (savedAlarmMelody !== null && alarmChangedByUser) {
                  // Keep user's draft
                  alarmMelodyField.value = savedAlarmMelody;
                } else if (!alarmChangedByUser) {
                  // Use server value
                  alarmMelodyField.value = data.alarmMelody || '';
                }
              }
              
              // Update timer melody field only if:
              // - User hasn't changed it from previous server value
              // - OR field is not focused and not being tested
              if (!timerMelodyTesting && 
                  (!timerChangedByUser || (document.activeElement !== timerMelodyField))) {
                // Check localStorage for drafts
                const savedTimerMelody = localStorage.getItem('timerMelodyDraft');
                if (savedTimerMelody !== null && timerChangedByUser) {
                  // Keep user's draft
                  timerMelodyField.value = savedTimerMelody;
                } else if (!timerChangedByUser) {
                  // Use server value
                  timerMelodyField.value = data.timerMelody || '';
                }
              }
            }
          } else {
            document.getElementById('melodyCard').style.display = 'none';
          }
          
          // Update alarm
          if (data.alarm.active) {
            const hourStr = (data.alarm.hour < 10 ? '0' : '') + data.alarm.hour;
            const minStr = (data.alarm.minute < 10 ? '0' : '') + data.alarm.minute;
            document.getElementById('alarmTime').value = hourStr + ':' + minStr;
            
            if (data.alarm.type === 'date') {
              document.getElementById('alarmType').value = 'date';
              document.getElementById('alarmDate').value = data.alarm.date;
            } else if (data.alarm.type === 'weekdays') {
              document.getElementById('alarmType').value = 'weekdays';
              const weekdayMask = data.alarm.weekdays;
              const checkboxes = document.querySelectorAll('#weekdaysField input[type="checkbox"]');
              checkboxes.forEach(cb => {
                const value = parseInt(cb.value);
                cb.checked = (weekdayMask & value) !== 0;
              });
            } else {
              document.getElementById('alarmType').value = 'daily';
            }
            
            if (data.alarm.text) {
              document.getElementById('alarmText').value = data.alarm.text;
            }
            
            document.getElementById('alarmRepeat').checked = data.alarm.repeat;
            document.getElementById('alarmSave').checked = data.alarm.saved;
            
            updateAlarmFields();
          }
          
          // Update alarm status
          const alarmDiv = document.getElementById('alarmStatus');
          if (data.alarm.active) {
            let alarmText = 'Alarm: ';
            if (data.alarm.type === 'date') alarmText += data.alarm.date + ' ';
            else if (data.alarm.type === 'weekdays') alarmText += 'Weekdays ';
            else alarmText += 'Daily ';
            alarmText += (data.alarm.hour < 10 ? '0' : '') + data.alarm.hour + ':';
            alarmText += (data.alarm.minute < 10 ? '0' : '') + data.alarm.minute;
            if (data.alarm.text) alarmText += ' "' + data.alarm.text + '"';
            if (data.alarm.repeat) alarmText += ' [R]';
            if (data.alarm.saved) alarmText += ' [S]';
            alarmDiv.textContent = alarmText;
            alarmDiv.className = 'alarm-info';
          } else {
            alarmDiv.textContent = 'No alarm set';
            alarmDiv.className = 'alarm-info inactive';
          }
          
          // Update timer status
          const timerDiv = document.getElementById('timerStatus');
          if (data.timer.active) {
            const min = Math.floor(data.timer.remaining / 60);
            const sec = data.timer.remaining % 60;
            let timerText = 'Timer: ' + min + ':' + (sec < 10 ? '0' : '') + sec + ' remaining';
            if (data.timer.text && data.timer.text !== 'TIMER') timerText += ' "' + data.timer.text + '"';
            timerDiv.textContent = timerText;
            timerDiv.className = 'timer-info';
            
            const hours = Math.floor(data.timer.remaining / 3600);
            const minutes = Math.floor((data.timer.remaining % 3600) / 60);
            const seconds = data.timer.remaining % 60;
            const hourStr = (hours < 10 ? '0' : '') + hours;
            const minStr = (minutes < 10 ? '0' : '') + minutes;
            const secStr = (seconds < 10 ? '0' : '') + seconds;
            document.getElementById('timerTime').value = hourStr + ':' + minStr + ':' + secStr;
            
            if (data.timer.text && data.timer.text !== 'TIMER') {
              document.getElementById('timerText').value = data.timer.text;
            }
          } else {
            timerDiv.textContent = 'No timer active';
            timerDiv.className = 'timer-info inactive';
          }
        })
        .catch(err => console.error('Status update error:', err));
    }
    
    function updateAlarmFields() {
      const type = document.getElementById('alarmType').value;
      document.getElementById('dateField').style.display = type === 'date' ? 'block' : 'none';
      document.getElementById('weekdaysField').style.display = type === 'weekdays' ? 'block' : 'none';
    }
    
    function setAlarm() {
      const type = document.getElementById('alarmType').value;
      const time = document.getElementById('alarmTime').value;
      const text = document.getElementById('alarmText').value;
      const repeat = document.getElementById('alarmRepeat').checked;
      const save = document.getElementById('alarmSave').checked;
      
      if (!time) {
        alert('Please set alarm time');
        return;
      }
      
      let url = '/alarm?time=' + time;
      url += '&type=' + type;
      
      if (type === 'date') {
        const date = document.getElementById('alarmDate').value;
        if (!date) {
          alert('Please select a date');
          return;
        }
        url += '&date=' + date;
      } else if (type === 'weekdays') {
        const checks = document.querySelectorAll('#weekdaysField input:checked');
        if (checks.length === 0) {
          alert('Please select at least one day');
          return;
        }
        let mask = 0;
        checks.forEach(c => mask += parseInt(c.value));
        url += '&weekdays=' + mask;
      }
      
      if (text) url += '&text=' + encodeURIComponent(text);
      if (repeat) url += '&repeat=1';
      if (save) url += '&save=1';
      
      fetch(url)
        .then(r => r.text())
        .then(data => {
          alert(data);
          updateStatus();
        })
        .catch(err => alert('Error: ' + err));
    }
    
    function clearAlarm() {
      fetch('/alarm/clear')
        .then(r => r.text())
        .then(data => {
          alert(data);
          updateStatus();
        })
        .catch(err => alert('Error: ' + err));
    }
    
    function setTimer() {
      const time = document.getElementById('timerTime').value;
      const text = document.getElementById('timerText').value;
      
      if (!time) {
        alert('Please set timer duration');
        return;
      }
      
      const parts = time.split(':');
      const hours = parseInt(parts[0]);
      const minutes = parseInt(parts[1]);
      const seconds = parseInt(parts[2] || 0);
      const totalSec = hours * 3600 + minutes * 60 + seconds;
      
      if (totalSec === 0) {
        alert('Timer must be at least 1 second');
        return;
      }
      
      let url = '/timer?duration=' + totalSec;
      if (text) url += '&text=' + encodeURIComponent(text);
      
      fetch(url)
        .then(r => r.text())
        .then(data => {
          alert(data);
          updateStatus();
        })
        .catch(err => alert('Error: ' + err));
    }
    
    function clearTimer() {
      fetch('/timer/clear')
        .then(r => r.text())
        .then(data => {
          alert(data);
          updateStatus();
        })
        .catch(err => alert('Error: ' + err));
    }
    
    function syncTime() {
      const ntp = document.getElementById('ntpServer').value;
      const tz = document.getElementById('timezone').value;
      const dst = document.getElementById('dstOffset').value;
      
      let url = '/sync';
      if (ntp) url += '?ntp=' + encodeURIComponent(ntp);
      if (tz) url += (url.includes('?') ? '&' : '?') + 'tz=' + tz;
      if (dst) url += '&dst=' + dst;
      
      fetch(url)
        .then(r => r.text())
        .then(data => {
          alert(data);
          updateStatus();
        })
        .catch(err => alert('Error: ' + err));
    }
    
    function setManualTime() {
      const time = document.getElementById('manualTime').value;
      if (!time) {
        alert('Please enter time in format: YYYY-MM-DD HH:MM:SS');
        return;
      }
      
      fetch('/time?value=' + encodeURIComponent(time))
        .then(r => r.text())
        .then(data => {
          alert(data);
          updateStatus();
        })
        .catch(err => alert('Error: ' + err));
    }
    
    function updateWeather() {
      const city = document.getElementById('cityName').value;
      fetch('/weather?city=' + encodeURIComponent(city))
        .then(r => r.text())
        .then(data => {
          alert(data);
        })
        .catch(err => alert('Error: ' + err));
    }
    
    function saveSettings() {
      const city = document.getElementById('cityName').value;
      const brightness = document.getElementById('brightness').value;
      
      let url = '/save?city=' + encodeURIComponent(city);
      url += '&brightness=' + brightness;
      
      fetch(url)
        .then(r => r.text())
        .then(data => {
          alert(data);
        })
        .catch(err => alert('Error: ' + err));
    }
    
    function restoreSettings() {
      if (confirm('Restore settings from NVS? This will reload the page.')) {
        fetch('/restore')
          .then(r => r.text())
          .then(data => {
            alert(data);
            setTimeout(() => location.reload(), 1000);
          })
          .catch(err => alert('Error: ' + err));
      }
    }
    
    function eraseSettings() {
      if (confirm('WARNING: This will erase ALL settings from NVS! Continue?')) {
        fetch('/erase')
          .then(r => r.text())
          .then(data => {
            alert(data);
          })
          .catch(err => alert('Error: ' + err));
      }
    }
    
    function rebootDevice() {
      if (confirm('Reboot the device?')) {
        fetch('/reboot')
          .then(r => r.text())
          .then(data => {
            alert('Device is rebooting... Please wait 10 seconds.');
            setTimeout(() => location.reload(), 10000);
          })
          .catch(err => alert('Error: ' + err));
      }
    }
    
    function testMelody(type) {
      let melody = '';
      if (type === 'alarm') {
        melody = document.getElementById('alarmMelody').value;
        alarmMelodyLastEdit = Date.now();  // Mark as recently edited
      } else {
        melody = document.getElementById('timerMelody').value;
        timerMelodyLastEdit = Date.now();  // Mark as recently edited
      }
      
      if (!melody) {
        alert('Please enter a melody first');
        return;
      }
      
      fetch('/melody/test?melody=' + encodeURIComponent(melody))
        .then(r => r.text())
        .then(data => {
          alert(data);
        })
        .catch(err => alert('Error: ' + err));
    }
    
    function saveMelodies() {
      const alarm = document.getElementById('alarmMelody').value;
      const timer = document.getElementById('timerMelody').value;
      
      let url = '/melody/save?alarm=' + encodeURIComponent(alarm);
      url += '&timer=' + encodeURIComponent(timer);
      
      fetch(url)
        .then(r => r.text())
        .then(data => {
          alert(data);
        })
        .catch(err => alert('Error: ' + err));
    }
    
    // Start status updates
    updateStatus();
    setInterval(updateStatus, 2000);
  </script>
</body>
</html>
)rawliteral";
  return html;
}

// ==================== WEB SERVER - SETUP ====================

/**
 * Setup web server routes
 */
void setupWebServer() {
  // Main page
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html; charset=utf-8", getWebPage());
  });
  
  // Status JSON
  server.on("/status", HTTP_GET, []() {
    server.send(200, "application/json; charset=utf-8", getStatusJSON());
  });
  
  // Weather update
  server.on("/weather", HTTP_GET, []() {
    if (server.hasArg("city")) {
      cityName = server.arg("city");
    }
    updateWeather();
    server.send(200, "text/plain; charset=utf-8", "Weather updated for " + cityName);
  });
  
  // Alarm set
  server.on("/alarm", HTTP_GET, []() {
    if (!timeValid) {
      server.send(400, "text/plain; charset=utf-8", "Wait for time sync!");
      return;
    }
    
    String timeStr = server.arg("time");
    String type = server.arg("type");
    
    if (timeStr.length() == 0) {
      server.send(400, "text/plain; charset=utf-8", "Missing time parameter");
      return;
    }
    
    int hour = timeStr.substring(0, 2).toInt();
    int minute = timeStr.substring(3, 5).toInt();
    
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
      server.send(400, "text/plain; charset=utf-8", "Invalid time");
      return;
    }
    
    myAlarm.active = true;
    myAlarm.hour = hour;
    myAlarm.minute = minute;
    myAlarm.repeat = server.hasArg("repeat");
    alarmTriggered = false;
    
    // Parse text
    String text = server.arg("text");
    if (text.length() > 0) {
      text.replace("+", " ");
      String decoded = "";
      for (int i = 0; i < text.length(); i++) {
        if (text[i] == '%' && i + 2 < text.length()) {
          String hex = text.substring(i + 1, i + 3);
          char c = strtol(hex.c_str(), NULL, 16);
          decoded += c;
          i += 2;
        } else {
          decoded += text[i];
        }
      }
      int byteCount = 0;
      for (int i = 0; i < decoded.length() && byteCount < 30; i++) {
        myAlarm.text[byteCount++] = decoded[i];
      }
      myAlarm.text[byteCount] = '\0';
    } else {
      myAlarm.text[0] = '\0';
    }
    
    // Parse date type
    if (type == "date") {
      String dateStr = server.arg("date");
      myAlarm.year = dateStr.substring(0, 4).toInt();
      myAlarm.month = dateStr.substring(5, 7).toInt();
      myAlarm.day = dateStr.substring(8, 10).toInt();
      myAlarm.weekdays = 0;
    } else if (type == "weekdays") {
      myAlarm.year = 0;
      myAlarm.month = 0;
      myAlarm.day = 0;
      myAlarm.weekdays = server.arg("weekdays").toInt();
    } else {
      myAlarm.year = 0;
      myAlarm.month = 0;
      myAlarm.day = 0;
      myAlarm.weekdays = 0;
    }
    
    if (server.hasArg("save")) {
      saveAlarmToNVS();
    } else {
      myAlarm.saved = false;
      updateLEDIndicator();
    }
    
    server.send(200, "text/plain; charset=utf-8", "Alarm set successfully");
  });
  
  // Alarm clear
  server.on("/alarm/clear", HTTP_GET, []() {
    memset(&myAlarm, 0, sizeof(myAlarm));
    alarmTriggered = false;
    buzzerActive = false;
    
    #if BUZZER_TYPE == BUZZER_PASSIVE
    stopMelody();
    #else
    digitalWrite(BUZZER_PIN, LOW);
    #endif
    
    clearAlarmFromNVS();
    updateLEDIndicator();
    server.send(200, "text/plain; charset=utf-8", "Alarm cleared");
  });
  
  // Timer set
  server.on("/timer", HTTP_GET, []() {
    String durationStr = server.arg("duration");
    
    if (durationStr.length() == 0) {
      server.send(400, "text/plain; charset=utf-8", "Missing duration parameter");
      return;
    }
    
    unsigned long totalSec = durationStr.toInt();
    
    if (totalSec == 0 || totalSec > 86400) {
      server.send(400, "text/plain; charset=utf-8", "Timer must be 1-86400 seconds");
      return;
    }
    
    // Parse text
    String text = server.arg("text");
    if (text.length() > 0) {
      text.replace("+", " ");
      String decoded = "";
      for (int i = 0; i < text.length(); i++) {
        if (text[i] == '%' && i + 2 < text.length()) {
          String hex = text.substring(i + 1, i + 3);
          char c = strtol(hex.c_str(), NULL, 16);
          decoded += c;
          i += 2;
        } else {
          decoded += text[i];
        }
      }
      int byteCount = 0;
      for (int i = 0; i < decoded.length() && byteCount < 30; i++) {
        timerText[byteCount++] = decoded[i];
      }
      timerText[byteCount] = '\0';
    } else {
      strcpy(timerText, "TIMER");
    }
    
    timerActive = true;
    timerTriggered = false;
    timerStartUs = esp_timer_get_time();
    timerDurationUs = (uint64_t)totalSec * 1000000;
    
    server.send(200, "text/plain; charset=utf-8", "Timer started");
  });
  
  // Timer clear
  server.on("/timer/clear", HTTP_GET, []() {
    timerActive = false;
    timerTriggered = false;
    buzzerActive = false;
    
    #if BUZZER_TYPE == BUZZER_PASSIVE
    stopMelody();
    #else
    digitalWrite(BUZZER_PIN, LOW);
    #endif
    
    server.send(200, "text/plain; charset=utf-8", "Timer cleared");
  });
  
  // Time sync
  server.on("/sync", HTTP_GET, []() {
    if (server.hasArg("ntp")) {
      ntpServer = server.arg("ntp");
    }
    if (server.hasArg("tz")) {
      gmtOffset_sec = server.arg("tz").toInt() * 3600;
    }
    if (server.hasArg("dst")) {
      daylightOffset_sec = server.arg("dst").toInt() * 3600;
    }
    
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer.c_str());
    delay(2000);
    
    if (getLocalTime(&timeinfo) && timeinfo.tm_year > 120) {
      timeValid = true;
      updateClockStrings();
      server.send(200, "text/plain; charset=utf-8", "Time synchronized successfully");
    } else {
      server.send(500, "text/plain; charset=utf-8", "Time synchronization failed");
    }
  });
  
  // Manual time set
  server.on("/time", HTTP_GET, []() {
    String timeStr = server.arg("value");
    if (timeStr.length() == 0) {
      server.send(400, "text/plain; charset=utf-8", "Missing time parameter");
      return;
    }
    
    struct tm t {};
    if (sscanf(timeStr.c_str(), "%d-%d-%d %d:%d:%d",
               &t.tm_year, &t.tm_mon, &t.tm_mday,
               &t.tm_hour, &t.tm_min, &t.tm_sec) == 6) {
      t.tm_year -= 1900;
      t.tm_mon -= 1;
      
      time_t tt = mktime(&t);
      struct timeval now = { tt, 0 };
      settimeofday(&now, nullptr);
      timeValid = true;
      updateClockStrings();
      server.send(200, "text/plain; charset=utf-8", "Manual time set successfully");
    } else {
      server.send(400, "text/plain; charset=utf-8", "Invalid time format. Use: YYYY-MM-DD HH:MM:SS");
    }
  });
  
  // Save settings
  server.on("/save", HTTP_GET, []() {
    if (server.hasArg("city")) {
      cityName = server.arg("city");
    }
    if (server.hasArg("brightness")) {
      int bright = server.arg("brightness").toInt();
      setDisplayBrightness(bright);
    }
    
    saveConfigToNVS();
    saveDisplaySettings();
    server.send(200, "text/plain; charset=utf-8", "Settings saved to NVS");
  });
  
  // Restore settings
  server.on("/restore", HTTP_GET, []() {
    loadConfigFromNVS();
    loadDisplaySettings();
    setDisplayBrightness(displayBrightness);
    connectWiFi();
    if (WiFi.status() == WL_CONNECTED) {
      syncTime();
    }
    updateClockStrings();
    server.send(200, "text/plain; charset=utf-8", "Settings restored from NVS");
  });
  
  // Erase NVS
  server.on("/erase", HTTP_GET, []() {
    eraseNVS();
    server.send(200, "text/plain; charset=utf-8", "NVS erased successfully");
  });
  
  // Reboot
  server.on("/reboot", HTTP_GET, []() {
    server.send(200, "text/plain; charset=utf-8", "Rebooting...");
    delay(1000);
    ESP.restart();
  });
  
  #if BUZZER_TYPE == BUZZER_PASSIVE
  // Melody test
  server.on("/melody/test", HTTP_GET, []() {
    String melody = server.arg("melody");
    if (melody.length() == 0) {
      server.send(400, "text/plain; charset=utf-8", "Missing melody parameter");
      return;
    }
    
    // Decode URL encoding
    melody.replace("+", " ");
    String decoded = "";
    for (int i = 0; i < melody.length(); i++) {
      if (melody[i] == '%' && i + 2 < melody.length()) {
        String hex = melody.substring(i + 1, i + 3);
        char c = strtol(hex.c_str(), NULL, 16);
        decoded += c;
        i += 2;
      } else {
        decoded += melody[i];
      }
    }
    
    // Test melody in non-blocking way
    Note testNotes[50];
    int testNoteCount = 0;
    parseMelody(decoded, testNotes, testNoteCount, 50);
    
    if (testNoteCount > 0) {
      // Play synchronously for testing
      for (int i = 0; i < testNoteCount; i++) {
        if (testNotes[i].frequency > 0) {
          buzzerTone(BUZZER_PIN, testNotes[i].frequency);
        } else {
          buzzerNoTone(BUZZER_PIN);
        }
        delay(testNotes[i].duration);
      }
      buzzerNoTone(BUZZER_PIN);
      server.send(200, "text/plain; charset=utf-8", "Melody tested");
    } else {
      server.send(400, "text/plain; charset=utf-8", "Invalid melody format");
    }
  });
  
  // Melody save
  server.on("/melody/save", HTTP_GET, []() {
    String alarm = server.arg("alarm");
    String timer = server.arg("timer");
    
    // Decode URL encoding for alarm
    if (alarm.length() > 0) {
      alarm.replace("+", " ");
      String decoded = "";
      for (int i = 0; i < alarm.length(); i++) {
        if (alarm[i] == '%' && i + 2 < alarm.length()) {
          String hex = alarm.substring(i + 1, i + 3);
          char c = strtol(hex.c_str(), NULL, 16);
          decoded += c;
          i += 2;
        } else {
          decoded += alarm[i];
        }
      }
      alarmMelody = decoded;
      parseMelody(alarmMelody, alarmNotes, alarmNoteCount, 50);
    }
    
    // Decode URL encoding for timer
    if (timer.length() > 0) {
      timer.replace("+", " ");
      String decoded = "";
      for (int i = 0; i < timer.length(); i++) {
        if (timer[i] == '%' && i + 2 < timer.length()) {
          String hex = timer.substring(i + 1, i + 3);
          char c = strtol(hex.c_str(), NULL, 16);
          decoded += c;
          i += 2;
        } else {
          decoded += timer[i];
        }
      }
      timerMelody = decoded;
      parseMelody(timerMelody, timerNotes, timerNoteCount, 50);
    }
    
    saveMelodySettings();
    server.send(200, "text/plain; charset=utf-8", "Melodies saved");
  });
  #endif
}

// ==================== MAIN LOOP ====================

/**
 * Arduino main loop
 * Handles all periodic tasks and state updates
 */
void loop() {
  // Handle web server
  server.handleClient();
  
  // Handle serial console
  handleSerial();
  
  // Handle button input
  handleButton();
  
  // Handle auto-return to clock screen
  handleAutoReturn();

  // Blink colon on clock display
  if (millis() - lastBlink >= 500) {
    colonVisible = !colonVisible;
    lastBlink = millis();
  }

  // Update weather periodically
  if (WiFi.status() == WL_CONNECTED && 
      millis() - lastWeatherUpdate > WEATHER_UPDATE_INTERVAL) {
    updateWeather();
  }

  // LED indicator animation for triggered states
  static unsigned long lastLedBlink = 0;
  static bool ledState = false;
  
  if (alarmTriggered) {
    // Blink red for alarm
    if (millis() - lastLedBlink >= 250) {
      ledState = !ledState;
      pixel.setPixelColor(0, ledState ? COLOR_ALARM_TRIG : COLOR_OFF);
      pixel.show();
      lastLedBlink = millis();
    }
  } else if (timerTriggered) {
    // Blink yellow for timer
    if (millis() - lastLedBlink >= 250) {
      ledState = !ledState;
      pixel.setPixelColor(0, ledState ? COLOR_TIMER_TRIG : COLOR_OFF);
      pixel.show();
      lastLedBlink = millis();
    }
  } else if (timerActive && myAlarm.active) {
    // Alternate between timer and alarm colors
    if (millis() - lastLedBlink >= 500) {
      ledState = !ledState;
      pixel.setPixelColor(0, ledState ? COLOR_TIMER : COLOR_ALARM);
      pixel.show();
      lastLedBlink = millis();
    }
  } else if (timerActive) {
    // Blink green for active timer
    if (millis() - lastLedBlink >= 500) {
      ledState = !ledState;
      pixel.setPixelColor(0, ledState ? COLOR_TIMER : COLOR_OFF);
      pixel.show();
      lastLedBlink = millis();
    }
  } else if (myAlarm.active) {
    // Solid blue for active alarm
    pixel.setPixelColor(0, COLOR_ALARM);
    pixel.show();
  } else {
    // Off when nothing active
    pixel.setPixelColor(0, COLOR_OFF);
    pixel.show();
  }

  // Update time validity
  if (getLocalTime(&timeinfo) && timeinfo.tm_year > 120) {
    timeValid = true;
  }

  // Update time strings every second
  static unsigned long lastSec = 0;
  if (timeValid && millis() - lastSec >= 1000) {
    lastSec = millis();
    updateClockStrings();

    // Check if alarm should trigger
    if (checkAlarmMatch()) {
      alarmTriggered = true;
      buzzerActive = true;
      currentScreen = SCREEN_ALARM;
      Serial.println("[Alarm] TRIGGERED!");
    }
  }

  // Check if timer expired
  if (timerActive && !timerTriggered) {
    uint64_t elapsed = esp_timer_get_time() - timerStartUs;
    if (elapsed >= timerDurationUs) {
      timerTriggered = true;
      buzzerActive = true;
      currentScreen = SCREEN_TIMER;
      Serial.println("[Timer] TRIGGERED!");
    }
  }

  // Handle buzzer control
  if (buzzerActive) {
    #if BUZZER_TYPE == BUZZER_PASSIVE
    // Play melody for passive buzzer
    if (!melodyPlaying) {
      if (alarmTriggered) {
        playMelody(alarmNotes, alarmNoteCount);
      } else if (timerTriggered) {
        playMelody(timerNotes, timerNoteCount);
      }
    }
    updateMelodyPlayback();
    #else
    // Simple beep pattern for active buzzer
    // Alarm: three short beeps
    // Timer: two short beeps with long pause
    uint64_t phase = esp_timer_get_time() % 2000000;  // 2 second cycle
    bool on = false;
    
    if (alarmTriggered) {
      // Three beeps pattern
      if (phase < 150000 || (phase >= 300000 && phase < 450000) || 
          (phase >= 600000 && phase < 750000)) {
        on = true;
      }
    } else if (timerTriggered) {
      // Two beeps with pause pattern
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

  // Update display based on current screen
  switch (currentScreen) {
    case SCREEN_INFO1:
      drawInfoScreen1();
      delay(100);
      break;

    case SCREEN_INFO2:
      drawInfoScreen2();
      delay(100);
      break;

    case SCREEN_INFO3:
      drawInfoScreen3();
      delay(100);
      break;

    case SCREEN_ALARM:
      if (alarmTriggered) {
        drawAlarmOrTimer(myAlarm.text);
        delay(20);
      } else {
        currentScreen = SCREEN_CLOCK;
      }
      break;

    case SCREEN_TIMER:
      if (timerTriggered) {
        drawAlarmOrTimer(timerText);
        delay(20);
      } else {
        currentScreen = SCREEN_CLOCK;
      }
      break;

    case SCREEN_CLOCK:
    default:
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
      delay(50);
      break;
  }
}
