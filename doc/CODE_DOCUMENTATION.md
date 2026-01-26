# ESP32-S3-Z-Clock Code Documentation

## File Structure Overview

```
ESP32-S3-Z-Clock.ino
├── Configuration & Constants (lines 1-450)
├── Core Functions (lines 451-2200)
├── Command Processing (lines 2201-3300)
├── Main Loop & Logic (lines 3301-3898)
```

---

## Configuration Section

### Hardware Pin Definitions
```cpp
#define BOOT_PIN 0              // Physical BOOT button for user input
#define BUZZER_PIN 12           // Passive/Active buzzer control
#define WS2812_PIN 21           // RGB LED strip (WS2812B)
#define NUM_PIXELS 1            // Number of LEDs in strip
#define SDA_PIN 10              // I2C data line for sensors
#define SCL_PIN 11              // I2C clock line for sensors
```

### Buzzer Configuration
```cpp
#define BUZZER_ACTIVE 0         // Active buzzer (just on/off)
#define BUZZER_PASSIVE 1        // Passive buzzer (can play tones/melodies)
#define BUZZER_TYPE BUZZER_PASSIVE
#define LOW_LEVEL_TRIGGER 1     // Buzzer trigger level (0=HIGH, 1=LOW)
```

### System Limits
```cpp
#define MAX_ALARMS 10           // Maximum number of configurable alarms
#define I2C_TIMEOUT_MS 100      // I2C operation timeout
#define I2C_MAX_RETRIES 3       // Number of I2C retry attempts
```

### Display Configuration
```cpp
const int DISP_W = 128;         // Display width in pixels
const int DISP_H = 64;          // Display height in pixels
const int YELLOW_ZONE_MAX = 20; // Yellow status bar area
const int BLUE_ZONE_MIN = 21;   // Blue display area start
```

### Default Settings
```cpp
String defSSID = "Enter_SSID";  // Default WiFi SSID (must be changed)
String defPASS = "Enter_Pass";  // Default WiFi password (must be changed)
String defNTP = "pool.ntp.org"; // Default NTP server for time sync
long defGMTOffset = 3 * 3600;   // Default timezone offset (GMT+3)
long defDaylightOffset = 0;     // Default DST offset (0 = no DST)
String defCity = "Moscow";      // Default city for weather
const int defBrightness = 255;  // Default display brightness (max)
```

### Melody Definitions
```cpp
// Default alarm melody - morning wake-up tone
String defAlarmMelody = "E5 E P S E5 E P S E5 Q P Q...";

// Default timer melody - completion signal
String defTimerMelody = "G6 E A6 E G6 E F6 E P W";

// Melody format: [Note][Octave] [Duration] [Pause] [Duration]
// Example: C5 Q = C note, octave 5, quarter note duration
// P = Pause, Q = Quarter duration pause
```

---

## Core Data Structures

### Translation System
```cpp
struct Translation {
  const char* en;  // English text
  const char* ru;  // Russian text
};

Translation translations[] = {
  {"Text in English", "Текст на русском"},
  // ... 300+ translation pairs
};

// Usage:
String TR(const char* text) {
  // Returns translated text based on currentLanguage setting
  // Falls back to English if translation not found
}
```

### Alarm Structure
```cpp
struct Alarm {
  bool active;              // Is alarm enabled?
  int hour;                 // Hour (0-23)
  int minute;               // Minute (0-59)
  bool repeat;              // Repeat alarm daily?
  uint8_t weekdays;         // Bitmask: bit 0=Mon, bit 6=Sun
  char text[51];            // Custom alarm text (max 50 chars)
  bool saved;               // Is alarm saved to NVS?
  char melody[201];         // Custom melody (max 200 chars)
  bool useDefaultMelody;    // Use default or custom melody?
  int year, month, day;     // For one-time alarms (0 = not set)
};

Alarm alarms[MAX_ALARMS];   // Array of 10 alarms
```

### Note Structure (for melodies)
```cpp
struct Note {
  int frequency;  // Frequency in Hz (0 = pause)
  int duration;   // Duration in milliseconds
};

Note alarmNotes[50];  // Parsed alarm melody
Note timerNotes[50];  // Parsed timer melody
Note customAlarmNotes[50];  // Custom alarm melody (per-alarm)
```

### WiFi Modes
```cpp
enum WiFiMode {
  WIFI_ALWAYS_ON = 0,  // WiFi always enabled
  WIFI_SMART = 1       // WiFi auto on/off for power saving
};
```

### Screen Modes
```cpp
enum ScreenMode {
  SCREEN_CLOCK = 0,    // Main clock display
  SCREEN_INFO1 = 1,    // Info screen (sensors, weather)
  SCREEN_INFO2 = 2,    // Info screen (WiFi, alarms)
  SCREEN_ALARM = 3,    // Alarm triggered screen
  SCREEN_TIMER = 4     // Timer triggered screen
};
```

---

## Key Functions

### Setup and Initialization

#### `setup()`
**Purpose:** Initialize all hardware and software components
**Called:** Once at device startup

**Main steps:**
1. Initialize Serial communication (115200 baud)
2. Initialize NVS (Non-Volatile Storage)
3. Load configuration from NVS
4. Initialize display (U8g2)
5. Initialize I2C sensors (AHT20, BMP280)
6. Initialize RGB LED
7. Initialize WiFi (connect or start AP)
8. Start web server
9. Sync time with NTP
10. Parse melodies
11. Display startup screen

```cpp
void setup() {
  Serial.begin(115200);
  initNVS();
  loadConfigFromNVS();
  u8g2.begin();
  initSensors();
  initLED();
  setupWiFi();
  startWebServer();
  syncTime();
  parseMelodies();
}
```

### Time Management

#### `syncTime()`
**Purpose:** Synchronize clock with NTP server
**Returns:** `bool` - true if sync successful

**Process:**
1. Configure timezone and DST
2. Call `configTime()` with NTP server
3. Wait for time sync (max 5 seconds)
4. Validate time (year > 2020)
5. Set `timeValid` flag

**Usage:**
```cpp
if (WiFi.status() == WL_CONNECTED) {
  if (syncTime()) {
    Serial.println("Time synced!");
  }
}
```

#### `updateClockStrings()`
**Purpose:** Update time/date display strings
**Called:** Every second in `loop()`

**Updates:**
- `hourStr`, `minStr`, `secStr` - Current time
- `dateStr` - Current date (DD.MM.YYYY)
- `weekdayStr` - Day of week name
- `timeinfo` structure

### Display Functions

#### `drawClock()`
**Purpose:** Render main clock screen on OLED display
**Called:** Every display refresh cycle

**Layout:**
```
┌────────────────────┐
│  Status Bar (top)  │  ← Yellow zone (sensors, WiFi)
├────────────────────┤
│                    │
│    12:34:56        │  ← Large time display
│   25.01.2026       │  ← Date
│   Saturday         │  ← Day of week
│                    │
│  Alarm/Timer Info  │  ← Bottom status
└────────────────────┘
```

**Display elements:**
- Top bar: Temperature, humidity, pressure, WiFi signal
- Center: Large time (HH:MM:SS)
- Below time: Date and day of week
- Bottom: Active alarms count, timer remaining

#### `drawInfo1()` / `drawInfo2()`
**Purpose:** Display information screens (sensors, weather, system status)
**Called:** When user cycles screens with button

**Info Screen 1:**
- Temperature (°C)
- Humidity (%)
- Pressure (hPa)
- Weather condition

**Info Screen 2:**
- WiFi status and signal
- Active alarms count
- Timer status
- System uptime

### Alarm System

#### `checkAlarms()`
**Purpose:** Check if any alarm should trigger
**Called:** Every second in `loop()`

**Logic:**
```cpp
for (int i = 0; i < MAX_ALARMS; i++) {
  if (!alarms[i].active) continue;
  
  // Check if current time matches alarm time
  if (hour == alarms[i].hour && minute == alarms[i].minute) {
    // Check weekday (if set)
    if (alarms[i].weekdays != 0) {
      int today = (timeinfo.tm_wday + 6) % 7; // Convert Sun=0 to Mon=0
      if (!(alarms[i].weekdays & (1 << today))) continue;
    }
    
    // Check date (for one-time alarms)
    if (alarms[i].year > 0) {
      if (year != alarms[i].year || month != alarms[i].month || 
          day != alarms[i].day) continue;
    }
    
    // Trigger alarm
    triggerAlarm(i);
  }
}
```

#### `triggerAlarm(int index)`
**Purpose:** Activate alarm at specified index
**Process:**
1. Set `alarmTriggered = true`
2. Set `buzzerActive = true`
3. Save alarm trigger time for auto-stop
4. Switch to alarm screen
5. Start melody playback (if passive buzzer)
6. If not repeat alarm, disable and save

#### `saveAlarmToNVS(int index)`
**Purpose:** Persist alarm configuration to NVS
**Storage format:**
```
NVS Key: "alarm_X" where X = index (0-9)
Data: JSON string with all alarm fields
```

### Timer System

#### `startTimer(int seconds, const char* text)`
**Purpose:** Start countdown timer
**Parameters:**
- `seconds` - Timer duration
- `text` - Optional description

**Implementation:**
```cpp
timerActive = true;
timerStartUs = esp_timer_get_time();  // Microsecond precision
timerDurationUs = seconds * 1000000ULL;
strncpy(timerText, text, 50);
```

**Advantages of microsecond timer:**
- No drift over long periods
- Precise timing even with delays in loop
- Uses ESP32 hardware timer

#### `checkTimer()`
**Purpose:** Check if timer has expired
**Called:** Every loop iteration

**Logic:**
```cpp
if (timerActive && !timerTriggered) {
  uint64_t elapsed = esp_timer_get_time() - timerStartUs;
  if (elapsed >= timerDurationUs) {
    timerTriggered = true;
    timerTriggerTime = millis();
    buzzerActive = true;
    currentScreen = SCREEN_TIMER;
  }
}
```

### Melody System (Passive Buzzer)

#### `parseMelody(String melody, Note* notes, int& count, int maxNotes)`
**Purpose:** Convert text melody to Note array
**Format:** `[Note][Octave] [Duration] [Pause] [Duration]`

**Example:**
```
Input:  "C5 Q D5 Q E5 H P Q"
Output: [
  {523, 250},  // C5 quarter note
  {587, 250},  // D5 quarter note
  {659, 500},  // E5 half note
  {0, 250}     // Pause quarter duration
]
```

**Note frequencies:**
```cpp
int noteFrequencies[12] = {
  262, 277, 294, 311, 330, 349,  // C, C#, D, D#, E, F
  370, 392, 415, 440, 466, 494   // F#, G, G#, A, A#, B
};
// Multiply by 2^(octave-4) for other octaves
```

#### `playMelody(Note* notes, int noteCount)`
**Purpose:** Start playing melody
**Process:**
1. Set `currentMelody` pointer
2. Set `melodyPlaying = true`
3. Start first note
4. Reset melody state

**Important:** Only called ONCE per alarm/timer event

#### `updateMelodyPlayback()`
**Purpose:** Handle melody note progression
**Called:** Every loop iteration when `melodyPlaying == true`

**State machine:**
```
PLAYING → (note duration elapsed) → NEXT NOTE
        → (melody end, timer) → STOP
        → (melody end, alarm) → PAUSE (1 sec) → REPEAT
PAUSE → (1 second elapsed) → PLAYING (from start)
```

**Key features:**
- Alarm: Repeats with 1-second pause between plays
- Timer: Plays once and stops
- Non-blocking (doesn't use `delay()`)

#### `stopMelody()`
**Purpose:** Stop melody playback immediately
**Actions:**
1. Set `melodyPlaying = false`
2. Call `buzzerNoTone()` to silence buzzer
3. Reset melody state variables

### Button Handling

#### `handleButton()`
**Purpose:** Process button presses with timing
**Called:** Every loop iteration

**Button states:**
- **Not pressed:** Reset counters
- **Short press (< 3 sec):** 
  - Stop alarm/timer
  - Enable screen (if off)
  - Cycle info screens
- **Long press (3-10 sec):**
  - Enable WiFi manually (Smart mode only)
- **Very long press (10+ sec):**
  - Enter setup mode (WiFi AP)

**Implementation:**
```cpp
unsigned long buttonPressStart = 0;
bool buttonPressed = false;

void handleButton() {
  bool currentState = (digitalRead(BOOT_PIN) == LOW);
  
  if (currentState && !buttonPressed) {
    // Button just pressed
    buttonPressed = true;
    buttonPressStart = millis();
  }
  else if (!currentState && buttonPressed) {
    // Button released
    unsigned long pressDuration = millis() - buttonPressStart;
    
    if (pressDuration < 3000) {
      // Short press
      handleShortPress();
    }
    else if (pressDuration < 10000) {
      // Long press
      handleLongPress();
    }
    else {
      // Very long press
      enterSetupMode();
    }
    
    buttonPressed = false;
  }
}
```

### WiFi Management

#### Smart WiFi Logic
**Purpose:** Automatic WiFi on/off for power saving

**Turns WiFi ON when:**
1. NTP sync needed (every 6 hours)
2. Weather update needed (every 1 hour)
3. Web request received (stays on 10 min)
4. Manual enable (button or command)

**Turns WiFi OFF when:**
1. No activity for 10 minutes
2. All scheduled tasks completed

**Implementation:**
```cpp
void manageSmartWiFi() {
  if (wifiMode != WIFI_SMART) return;
  
  bool shouldBeOn = false;
  unsigned long now = millis();
  
  // Check if WiFi should be on
  if (wifiManuallyEnabled && now < wifiManualTimeout) shouldBeOn = true;
  if (now - lastWebRequest < wifiAutoOffTimeout) shouldBeOn = true;
  if (timeValid && now - lastNtpSync >= ntpSyncInterval) shouldBeOn = true;
  if (timeValid && now - lastWeatherSync >= weatherSyncInterval) shouldBeOn = true;
  
  // Turn on/off as needed
  if (shouldBeOn && !wifiActive) turnWiFiOn();
  else if (!shouldBeOn && wifiActive) turnWiFiOff();
}
```

**Power savings:**
- WiFi off: ~30-50mA reduction
- Typical on-time: 1-2 minutes per hour
- Average current: ~80mA vs ~130mA always-on

### Sensor Reading

#### `readSensorData()`
**Purpose:** Read temperature, humidity, pressure from I2C sensors
**Called:** Every 2 seconds

**Thread safety:**
```cpp
void readSensorData() {
  if (readingSensors) return;  // Skip if already reading
  readingSensors = true;
  
  // Take I2C mutex
  if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) == pdTRUE) {
    // Read AHT20
    if (ahtFound) {
      sensors_event_t humidity, temp;
      aht.getEvent(&humidity, &temp);
      temperature = temp.temperature;
      humidityVal = humidity.relative_humidity;
    }
    
    // Read BMP280 (custom implementation for reliability)
    if (bmpFound) {
      pressure = readBMP280Pressure();
    }
    
    xSemaphoreGive(i2cMutex);
  }
  
  readingSensors = false;
}
```

**Why custom BMP280 implementation:**
- Adafruit library had reliability issues
- Direct I2C register access more stable
- Compensates for temperature in pressure reading

### LED Indicator

#### `updateLEDIndicator()`
**Purpose:** Set RGB LED color based on system state
**Called:** When state changes

**Color coding:**
```cpp
void updateLEDIndicator() {
  if (alarmTriggered) {
    setLED(255, 0, 0);      // Red - alarm ringing
  }
  else if (timerTriggered) {
    setLED(0, 255, 0);      // Green - timer expired
  }
  else if (setupMode) {
    setLED(128, 0, 128);    // Purple - setup mode
  }
  else if (wifiConnecting) {
    setLED(255, 255, 0);    // Yellow - connecting
  }
  else if (wifiConnected) {
    setLED(0, 0, 255);      // Blue - connected
  }
  else {
    setLED(0, 0, 0);        // Off - normal operation
  }
}
```

### Night Mode

#### `updateNightMode()`
**Purpose:** Auto-adjust brightness based on time
**Called:** Every minute

**Logic:**
```cpp
void updateNightMode() {
  if (!nightModeEnabled) return;
  
  int currentMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
  int startMinutes = nightStartHour * 60 + nightStartMinute;
  int endMinutes = nightEndHour * 60 + nightEndMinute;
  
  bool shouldBeActive;
  if (startMinutes < endMinutes) {
    // Normal case: 22:00-07:00
    shouldBeActive = (currentMinutes >= startMinutes && 
                      currentMinutes < endMinutes);
  } else {
    // Wraps midnight: 23:00-01:00
    shouldBeActive = (currentMinutes >= startMinutes || 
                      currentMinutes < endMinutes);
  }
  
  if (shouldBeActive && !nightModeActive) {
    // Enter night mode
    nightModeActive = true;
    setDisplayBrightness(nightBrightness);
  }
  else if (!shouldBeActive && nightModeActive) {
    // Exit night mode
    nightModeActive = false;
    setDisplayBrightness(displayBrightness);
  }
}
```

### Command Processing

#### `handleSerialCommands()`
**Purpose:** Process commands from Serial console
**Called:** Every loop iteration

**Command parser:**
```cpp
void handleSerialCommands() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();  // Commands are case-insensitive
    
    // Parse and execute command
    if (cmd.equals("STATUS")) showStatus();
    else if (cmd.equals("HELP")) showHelp();
    else if (cmd.startsWith("WIFI ")) handleWiFiCommand(cmd);
    else if (cmd.startsWith("ALARM ")) handleAlarmCommand(cmd);
    // ... etc
    
    Serial.print("> ");  // Prompt for next command
  }
}
```

**Command categories:**
1. System: STATUS, HELP, SAVE, RESTORE, REBOOT
2. Time: TIME, SYNC, TZ, DST, NTP
3. WiFi: WIFI SET/ON/OFF/STATUS/ALWAYS/SMART
4. Display: BRIGHTNESS, NIGHT
5. Alarms: ALARM LIST/SET/CLEAR/TOGGLE
6. Timer: TIMER [duration] [text], TIMER CLEAR
7. Melodies: MELODY ALARM/TIMER/TEST/SAVE
8. Language: LANGUAGE EN/RU

---

## Main Loop

### `loop()` execution order

```cpp
void loop() {
  // 1. Critical timing - every iteration
  unsigned long now = millis();
  handleButton();
  
  // 2. WiFi management
  if (wifiMode == WIFI_SMART) manageSmartWiFi();
  
  // 3. Time and date - every second
  static unsigned long lastSecond = 0;
  if (now - lastSecond >= 1000) {
    updateClockStrings();
    checkAlarms();
    lastSecond = now;
  }
  
  // 4. Sensors - every 2 seconds
  static unsigned long lastSensor = 0;
  if (now - lastSensor >= 2000) {
    readSensorData();
    lastSensor = now;
  }
  
  // 5. Display - every 100ms
  static unsigned long lastDisplay = 0;
  if (now - lastDisplay >= 100) {
    drawCurrentScreen();
    lastDisplay = now;
  }
  
  // 6. Melody playback - continuous when active
  if (buzzerActive && melodyPlaying) {
    updateMelodyPlayback();
  }
  
  // 7. Auto-stop alarms/timers - every second
  checkAutoStop();
  
  // 8. Weather updates - when needed
  if (needWeatherUpdate && wifiConnected) {
    updateWeather();
  }
  
  // 9. Serial commands - when available
  handleSerialCommands();
  
  // 10. Night mode - every minute
  static unsigned long lastNightCheck = 0;
  if (now - lastNightCheck >= 60000) {
    updateNightMode();
    lastNightCheck = now;
  }
}
```

### Loop timing considerations

**Fast operations (every loop):**
- Button reading (~10 µs)
- WiFi state machine (~50 µs)
- Total loop overhead: < 1 ms

**Medium operations (every 100-1000 ms):**
- Display rendering (~20 ms)
- Time updates (~1 ms)
- Melody updates (~0.1 ms)

**Slow operations (every 2-60 seconds):**
- Sensor readings (~50 ms with I2C)
- Night mode checks (~0.1 ms)

**Very slow operations (on-demand):**
- NTP sync (1-2 seconds)
- Weather API (2-5 seconds)
- Web requests (varies)

**Design principles:**
1. No `delay()` in main loop
2. All operations non-blocking
3. State machines for long operations
4. Careful timing to avoid overlap

---

## Memory Management

### Heap Usage
```cpp
// Typical heap usage:
// - Base system: ~50 KB
// - WiFi stack: ~30 KB
// - Web server: ~20 KB
// - Display buffers: ~10 KB
// - String buffers: ~5 KB
// Total: ~115 KB of ~320 KB available
```

### Stack Usage
```cpp
// Main task stack: 8 KB (default)
// Careful with:
// - Large local arrays (use global or heap)
// - Deep recursion (avoid)
// - String concatenation (use minimal)
```

### NVS Usage
```cpp
// NVS partitions:
// - nvs: System settings
// - prefs: Application settings
// Total usage: ~2-3 KB of 20 KB available
```

---

## Error Handling

### I2C Communication
```cpp
// Retry logic for sensor reading:
for (int retry = 0; retry < I2C_MAX_RETRIES; retry++) {
  if (readSensor()) break;
  delay(10);
}
```

### WiFi Connection
```cpp
// Timeout and retry logic:
int attempts = 0;
while (WiFi.status() != WL_CONNECTED && attempts < 20) {
  delay(500);
  attempts++;
}
if (WiFi.status() != WL_CONNECTED) {
  Serial.println("WiFi connection failed!");
}
```

### NVS Operations
```cpp
// Check NVS operations:
esp_err_t err = nvs_flash_init();
if (err != ESP_OK) {
  Serial.println("NVS init failed!");
  nvs_flash_erase();  // Try recovery
  nvs_flash_init();
}
```

---

## Performance Optimization

### Display Rendering
```cpp
// Use U8g2 full buffer mode for faster updates
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(...);
// F = Full buffer mode (faster, uses more RAM)
```

### String Operations
```cpp
// Minimize String concatenation:
// Bad: String s = "a" + "b" + "c" + "d";
// Good: String s; s.reserve(10); s += "a"; s += "b"; ...
```

### Melody Parsing
```cpp
// Parse melodies once at startup, not every playback
parseMelody(alarmMelody, alarmNotes, alarmNoteCount, 50);
// Notes stored in array for fast access
```

---

## Debugging Tips

### Serial Debug
```cpp
// Debug timing:
unsigned long start = millis();
doSomething();
Serial.printf("Operation took %lu ms\n", millis() - start);

// Debug memory:
Serial.printf("Free heap: %d KB\n", esp_get_free_heap_size() / 1024);

// Debug I2C:
Serial.printf("AHT20: %s, BMP280: %s\n", 
              ahtFound ? "OK" : "NOT FOUND",
              bmpFound ? "OK" : "NOT FOUND");
```

### LED Debug
```cpp
// Use LED for visual debugging:
setLED(255, 0, 0);     // Red = error
delay(100);
setLED(0, 255, 0);     // Green = success
```

### Common Issues

**1. Display not updating**
- Check loop timing
- Verify display buffer clear
- Check I2C mutex locks

**2. Alarms not triggering**
- Verify time is synced (timeValid == true)
- Check alarm active flag
- Verify time comparison logic

**3. WiFi disconnecting**
- Check Smart WiFi logic
- Verify auto-off timeout
- Check signal strength

**4. Melody not playing**
- Verify buzzerActive flag
- Check melody parsing
- Verify buzzer pin and type

**5. Sensors not reading**
- Check I2C connections
- Verify addresses (AHT20=0x38, BMP280=0x76)
- Check mutex deadlocks

---

## Future Enhancements

### Possible Additions
1. **MQTT Support** - For smart home integration
2. **OTA Updates** - Firmware updates over WiFi
3. **Touch Display** - Replace button with touchscreen
4. **Battery Monitor** - For portable operation
5. **More Sensors** - Light, motion, air quality
6. **Custom Widgets** - User-defined display screens
7. **Alarm Snooze** - Temporary alarm delay
8. **Gradual Wake** - Slowly increase brightness/volume
9. **Weather Forecast** - Multi-day forecast display
10. **Calendar Events** - Google Calendar integration

### Code Improvements
1. **Modular Design** - Split into multiple files
2. **Configuration File** - JSON config instead of NVS
3. **State Machine** - Formal state machine for modes
4. **Unit Tests** - Automated testing framework
5. **Memory Pools** - Pre-allocated memory management
6. **Logging System** - Structured logging with levels
