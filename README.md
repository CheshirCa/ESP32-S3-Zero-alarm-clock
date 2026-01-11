# ESP32-S3-Zero Alarm Clock / Будильник на ESP32-S3-Zero

[English](#english) | [Русский](#russian)

---

<a name="english"></a>
## English

### Advanced Alarm Clock and Timer System for ESP32-S3-Zero

A feature-rich alarm clock built on the ESP32-S3-Zero board with OLED display, WiFi connectivity, NTP time synchronization, weather display, customizable buzzer alerts with melody support, and dual-zone display.


---

### Features

####  Alarm System
- **Daily alarms** - repeat every day at specified time
- **Weekday-specific alarms** - set different alarms for weekdays (Monday-Sunday)
- **Date-specific alarms** - one-time alarms for specific dates
- **Custom alarm text** - display personalized messages (up to 30 characters, supports Cyrillic)
- **Repeat option** - alarm repeats after being triggered
- **NVS storage** - save alarms to non-volatile memory

####  Timer System
- **Countdown timer** - set timers from 1 second to 24 hours
- **Custom timer text** - personalized timer labels
- **Visual feedback** - LED indicator shows timer status
- **Microsecond precision** - accurate timing with esp_timer

####  Advanced Buzzer Control
- **Active buzzer support** - simple buzzers with built-in oscillator (HCM1203X) - ON/OFF only
- **Passive buzzer support** - piezoelectric elements with melody playback via PWM
- **Low/High level trigger** - configurable for different buzzer types (MH-FMD uses LOW level)
- **Custom melodies** - create and save your own melodies for alarm and timer (passive buzzer only)
- **Melody format** - simple note notation (e.g., "C5 Q D5 Q E5 H")
- **Melody testing** - test melodies before saving
- **Different patterns** - different beep patterns for alarm and timer (active buzzer)

####  Network Features
- **WiFi connectivity** - connect to your wireless network
- **Web interface** - control clock from any browser on your network
- **NTP time sync** - automatic time synchronization with internet time servers
- **Weather display** - shows current weather from wttr.in
- **Configurable city** - set your location for accurate weather

####  Advanced Display Features
- **Dual-color OLED** - 0.96" 128x64 SSD1306 display
  - Yellow zone (Y: 0-20) - status bar with date and indicators
  - Blue zone (Y: 21-64) - main content area with auto-scaling text
- **Auto font sizing** - text automatically scales to fit display area
- **Word wrapping** - intelligent text wrapping for long messages
- **Multiple info screens** - cycle through system info, network info, and weather
- **Adjustable brightness** - set display contrast (0-255)
- **Cyrillic support** - full support for Russian characters
- **Splash screen** - custom startup screen

####  Storage
- **Non-volatile storage (NVS)** - settings persist across reboots
- **Saves all settings**:
  - WiFi credentials
  - Time zone and DST settings
  - Alarm configuration
  - Display brightness
  - Custom melodies (passive buzzer)
  - Weather city

####  Control Options
- **Web interface** - full control from browser with real-time updates
- **Serial console** - command-line interface via USB with command history
- **Physical button** - BOOT button for navigation and alarm stop
- **Command history** - arrow keys navigate previous commands (serial console)
- **Auto-return to clock** - info screens automatically return after timeout

####  LED Indicator
- **WS2812 RGB LED** - visual status indicator
- **Color coding**:
  - Blue: Alarm set
  - Green: Timer active (blinking)
  - Red: Alarm triggered (fast blinking)
  - Yellow: Timer triggered (fast blinking)
  - Purple: Both alarm and timer active (alternating)
- **Built-in** - no external wiring needed

---

### Hardware Requirements

#### Main Board
- **ESP32-S3-Zero** (Waveshare)
  - ESP32-S3 microcontroller
  - Built-in WS2812 RGB LED (GPIO21)
  - BOOT button (GPIO0)
  - USB-C connector for programming and power

#### Display
- **SSD1306 OLED Display**
  - Size: 0.96" (128x64 pixels)
  - Interface: I2C
  - Color: yellow/blue zones
  - Connections:
    - SCL → GPIO5
    - SDA → GPIO6
    - VCC → 3.3V
    - GND → GND

#### Buzzer (Choose ONE)
**⚠️ IMPORTANT: Connect to 3.3V, NOT 5V**

**Option 1: Active Buzzer (Recommended for beginners)**
- Buzzer with built-in oscillator (HCM1203X)
- Simple ON/OFF operation only
- Cannot play melodies
- Connections:
  - Positive → GPIO12
  - Negative → GND

**Option 2: Passive Buzzer (For melody support)**
- Piezoelectric element without oscillator
- Controlled via PWM frequency
- Supports custom melodies
- **Voltage: 3.3V signal** - Use voltage divider if connecting to 5V source
- Connections:
  - Signal → GPIO12 (via transistor module or direct)
  - VCC → 3.3V (use resistor divider for 5V buzzers)
  - GND → GND

**Low-Level Trigger Buzzers (like MH-FMD):**
- Some passive buzzers activate on LOW signal
- Set `#define LOW_LEVEL_TRIGGER 1` in code
- Connections same as above

#### Connections Summary
```
ESP32-S3-Zero Pinout:
GPIO0  - BOOT button (built-in)
GPIO5  - OLED SCL (I2C clock)
GPIO6  - OLED SDA (I2C data)
GPIO12 - Buzzer output (PWM capable)
GPIO21 - WS2812 LED (built-in)
3.3V   - Power for OLED and buzzer
GND    - Common ground
```

---

### Software Requirements

#### Arduino IDE Setup
1. **Install Arduino IDE** (version 2.0+ recommended)
2. **Add ESP32 board support**:
   - File → Preferences
   - Additional Board Manager URLs: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
   - Tools → Board → Boards Manager
   - Search for "esp32" and install "esp32 by Espressif Systems"

3. **Select board**:
   - Tools → Board → ESP32 Arduino → Waveshare ESP32 S3 Zero

4. **Configure board settings**:
   ```
   Board: "Waveshare ESP32 S3 Zero"
   USB CDC On Boot: "Enabled"
   CPU Frequency: "240MHz (WiFi)"
   Flash Size: "4MB (32Mb)"
   Partition Scheme: "Default 4MB with spiffs"
   PSRAM: "Disabled"
   Upload Speed: "921600"
   ```

#### Required Libraries
Install these libraries via Arduino Library Manager (Sketch → Include Library → Manage Libraries):

1. **U8g2** (by oliver) - OLED display driver
   - Version: 2.35.0 or later
   - Search: "u8g2"
   - Required for: Display control, Cyrillic fonts

2. **Adafruit NeoPixel** (by Adafruit) - WS2812 LED control
   - Version: 1.12.0 or later
   - Search: "adafruit neopixel"
   - Required for: RGB LED control

3. **Built-in ESP32 libraries** (no installation needed):
   - WiFi
   - WebServer
   - HTTPClient
   - Preferences
   - time.h functions

---

### Installation and Configuration

#### 1. Download and Open
```bash
git clone https://github.com/CheshirCa/ESP32-S3-Zero-alarm-clock.git
cd ESP32-S3-Zero-alarm-clock
```
Open `ESP32-C3-Z-Clock.ino` in Arduino IDE.

#### 2. Configure Buzzer Type (CRITICAL STEP)
In the sketch, around lines 40-55, configure for YOUR hardware:

**For Active Buzzer (simple beeps):**
```cpp
#define BUZZER_TYPE BUZZER_ACTIVE  // Use active buzzer
#define LOW_LEVEL_TRIGGER 0        // Standard HIGH trigger
```

**For Passive Buzzer (melodies):**
```cpp
#define BUZZER_TYPE BUZZER_PASSIVE  // Use passive buzzer
#define LOW_LEVEL_TRIGGER 0         // Standard HIGH trigger
```

**For MH-FMD or Low-Level Trigger Buzzers:**
```cpp
#define BUZZER_TYPE BUZZER_PASSIVE  // Use passive buzzer
#define LOW_LEVEL_TRIGGER 1         // LOW level trigger
```

#### 3. Upload to Board
1. Connect ESP32-S3-Zero via USB-C
2. Select correct COM port (Tools → Port)
3. Click Upload button (→)
4. Wait for "Done uploading" message

#### 4. Initial Setup via Serial Console
1. Open Serial Monitor (Tools → Serial Monitor, 115200 baud)
2. Wait for boot message: "=== ESP32-S3-Zero Alarm Clock ==="
3. Configure basic settings:
   ```
   > WIFI YourSSID YourPassword
   > CITY Moscow
   > TZ +3
   > SAVE
   ```
4. Device will connect to WiFi and sync time automatically

---

### Usage Guide

#### First Boot Sequence
1. Splash screen shows author info
2. OLED test pattern appears
3. Configuration loads from NVS (or defaults)
4. WiFi connection attempt (if configured)
5. NTP time synchronization
6. Weather update (if WiFi connected)
7. Main clock display appears

#### Web Interface
1. **Find IP Address**:
   - Check Serial Monitor: "[WebServer] Access at: http://192.168.x.x"
   - Or press BOOT button to cycle to Info Screen 2

2. **Access Interface**:
   - Open browser on any device connected to same WiFi
   - Navigate to: `http://<device_ip>`
   - No password required

3. **Web Interface Features**:
   - **Real-time Clock** - Large time display with auto-update
   - **System Settings** - WiFi, NTP, timezone, brightness
   - **Alarm Management** - Set daily/weekly/date-specific alarms
   - **Timer Control** - Start countdown timers
   - **Weather Settings** - Configure city and update weather
   - **Melody Editor** - Create and test custom melodies (passive buzzer only)
   - **System Actions** - Save, restore, erase, reboot

4. **Melody Editor Tips**:
   - Enter melodies in format: "C5 Q D5 Q E5 H"
   - Click "Test" to hear before saving
   - Use "Save Melodies" to store in NVS
   - Melodies persist after reboot

#### Serial Console Commands

##### General Commands
- `HELP` - Show all available commands
- `STATUS` - Display detailed system status
- `REBOOT` - Restart the device

##### Time Management
- `TIME 2026-01-15 14:30:00` - Set manual time (YYYY-MM-DD HH:MM:SS)
- `SYNC` - Force NTP time synchronization
- `TZ +3` - Set timezone offset (GMT+3 for Moscow)
- `DST +0` - Set daylight saving offset (0 for Russia)
- `NTP pool.ntp.org` - Change NTP server

##### Network Configuration
- `WIFI MySSID MyPassword` - Set WiFi credentials
- `CITY London` - Set weather city (supports international names)

##### Display Control
- `BRIGHTNESS 200` - Set OLED contrast (0-255, 255=max)
- `DISPLAY TEST` - Show test pattern

##### Alarm Commands
```
ALARM [YYYY-MM-DD|1234567] HH:MM [TEXT] [R] [S]
```
- **Date formats**:
  - `07:30` - Daily at 7:30 AM
  - `2026-12-31 23:59` - Specific date
  - `12345 08:00` - Weekdays Mon-Fri at 8:00
- **Flags**:
  - `R` - Repeat after triggering
  - `S` - Save to NVS (persist across reboots)
- **Examples**:
  - `ALARM 07:30 Wake up! R S` - Daily alarm, repeat, save
  - `ALARM 2026-12-31 23:59 Happy New Year! S` - New Year's alarm
  - `ALARM 67 09:00 Weekend S` - Saturday & Sunday at 9:00
  - `ALARM CLEAR` - Remove current alarm

##### Timer Commands
```
TIMER [HH:]MM:SS [TEXT]
TIMER SS [TEXT]
```
- **Formats**:
  - `300` - 300 seconds (5 minutes)
  - `05:00` - 5 minutes
  - `01:30:00` - 1 hour 30 minutes
- **Examples**:
  - `TIMER 300 Tea ready` - 5-minute timer
  - `TIMER 45:00 Pizza` - 45-minute timer
  - `TIMER CLEAR` - Stop current timer

##### Storage Management
- `SAVE` - Save ALL current settings to NVS
- `RESTORE` - Load settings from NVS
- `ERASE` - Erase ALL NVS data (factory reset)

##### Buzzer and Melody Commands (Passive Buzzer Only)
- `MELODY ALARM C5 Q D5 Q E5 H` - Set alarm melody
- `MELODY TIMER C5 E C5 E P H` - Set timer melody
- `MELODY TEST C5 Q D5 Q E5 H` - Test a melody
- `MELODY SAVE` - Save melodies to NVS

**Melody Format**:
- **Notes**: `C`, `C#`, `D`, `D#`, `E`, `F`, `F#`, `G`, `G#`, `A`, `A#`, `B`, `P` (pause)
- **Octaves**: `3`, `4`, `5`, `6`, `7` (middle C = C4)
- **Durations**: `W` (whole=1000ms), `H` (half=500ms), `Q` (quarter=250ms), `E` (eighth=125ms), `S` (sixteenth=63ms)
- **Example Melodies**:
  - Simple beep: `C5 Q P Q C5 Q`
  - Alert: `E5 E E5 E E5 Q G5 Q C5 Q D5 Q E5 H`
  - Timer beep: `C5 E C5 E P H`

#### Button Operation
- **Single press**: Cycle through screens (Clock → Info1 → Info2 → Info3 → Clock)
- **When alarm/timer triggers**: Press to stop sound and return to clock
- **Auto-return**: Info screens return to clock after 10 seconds

#### LED Indicator Colors
- **Solid Blue** - Alarm is set
- **Blinking Green** - Timer is active (500ms interval)
- **Fast Blinking Red** - Alarm triggered (250ms interval)
- **Fast Blinking Yellow** - Timer triggered (250ms interval)
- **Alternating Blue/Green** - Both alarm and timer active
- **Off** - No alarms or timers

---

### Display Screens

#### Main Clock Screen
- Large digital time (38px font) with blinking colon
- Date in top yellow zone (DD.MM.YYYY format)
- Indicators: `A` (alarm active), `T` (timer active)
- Time sync status (colon blinks only when synced)

#### Info Screen 1 - System Status
- Current weekday
- Alarm status with time if active
- Timer status with remaining time
- WiFi connection status
- Auto-returns to clock after 10 seconds

#### Info Screen 2 - Network Information
- WiFi SSID (truncated if long)
- IP address
- Time synchronization status
- Free RAM in kilobytes

#### Info Screen 3 - Weather Display
- City name
- Current temperature (°C)
- Weather conditions
- Auto-updates every 10 minutes
- Shows "No data" if WiFi not connected

#### Alarm/Timer Triggered Screen
- Full-screen custom message
- "Press BOOT!" instruction in yellow zone
- Message auto-scales to fit blue zone
- Different fonts tried from largest to smallest
- Supports Cyrillic text

---

### Configuration Examples

#### Example 1: Complete Morning Routine
```
Serial Console Setup:
> WIFI HomeWiFi MyPassword
> CITY Moscow
> TZ +3
> NTP ru.pool.ntp.org
> ALARM 12345 07:00 Wake up! R S
> ALARM 67 08:30 Weekend R S
> BRIGHTNESS 150
> CITY Moscow
> SAVE

Web Interface:
- Set display brightness to 150
- Verify timezone GMT+3
- Test alarm melody
- Set weather update frequency
```

#### Example 2: Cooking Assistant
```
> TIMER 15:00 Check pasta
[15 minutes later - timer triggers]
Display shows: "CHECK PASTA" in large font
Buzzer plays timer melody
Press BOOT button to stop

Web Interface:
- Create multiple timers sequentially
- Use descriptive text: "Tea", "Pizza", "Cake"
- Save frequently used timers as presets
```

#### Example 3: Custom Melody Creation
```
For Passive Buzzer:
> MELODY ALARM E5 E P S E5 E P S E5 Q P Q E5 E P S E5 E P S E5 Q P Q
> MELODY TEST E5 E P S E5 E P S E5 Q P Q
> MELODY SAVE

Web Interface Melody Editor:
- Enter: C5 Q D5 Q E5 Q F5 Q G5 Q A5 Q B5 Q C6 H
- Click "Test Alarm Melody" to preview
- Click "Save Melodies" to store
- Melody saved to NVS, persists after reboot
```

#### Example 4: Special Occasions
```
Birthday Reminder:
> ALARM 2026-07-15 09:00 Happy Birthday! S

Meeting Alarm:
> ALARM 35 14:30 Team Meeting S  (Tue/Thu at 2:30 PM)

Meditation Timer:
> TIMER 20:00 Meditation
> ALARM 1234567 21:00 Evening meditation R S
```

---

### Melody Creation Guide

#### Understanding the Format
Melodies use a simple space-separated format: `NoteOctave Duration`
- `E5` - E note, 5th octave (659 Hz)
- `Q` - Quarter note (250ms)
- `P` - Pause (silence)
- `S` - Sixteenth note (63ms)

#### Note Frequencies
| Note | Octave 4 | Octave 5 | Octave 6 |
|------|----------|----------|----------|
| C    | 262 Hz   | 523 Hz   | 1047 Hz  |
| D    | 294 Hz   | 587 Hz   | 1175 Hz  |
| E    | 330 Hz   | 659 Hz   | 1319 Hz  |
| F    | 349 Hz   | 698 Hz   | 1397 Hz  |
| G    | 392 Hz   | 784 Hz   | 1568 Hz  |
| A    | 440 Hz   | 880 Hz   | 1760 Hz  |
| B    | 494 Hz   | 988 Hz   | 1976 Hz  |

#### Popular Melodies

**Simple Alert** (good for alarms):
```
E5 E P S E5 E P S E5 Q P Q
```
Translation: Two short beeps, pause, one longer beep

**Timer Beep**:
```
C5 E C5 E P H
```
Translation: Two quick beeps, long pause, repeat

**Ascending Scale** (attention grabber):
```
C5 Q D5 Q E5 Q F5 Q G5 Q A5 Q B5 Q C6 H
```

**Custom Tune**:
```
E5 Q G5 Q E5 Q C5 Q D5 Q E5 H P H
```

#### Tips for Better Melodies
1. **Start simple** - Use 3-5 notes for basic alerts
2. **Add pauses** - Pauses create rhythm: `C5 Q P Q C5 Q`
3. **Mix durations** - Combine long and short notes
4. **Test often** - Use `MELODY TEST` before saving
5. **Consider volume** - Higher octaves (6,7) are louder
6. **Keep it short** - 10-20 notes maximum for clarity

---

### Troubleshooting

#### Device Won't Boot or Crashes
**Symptoms**: Continuous reset, "Guru Meditation Error", blank display
**Solutions**:
1. **Check power supply**:
   - Use quality USB cable (data + power)
   - Avoid USB hubs, connect directly to computer
   - Ensure 5V 1A minimum power supply

2. **Verify wiring**:
   - OLED SCL → GPIO5, SDA → GPIO6
   - Buzzer → GPIO12 (check polarity)
   - All grounds connected together

3. **Code configuration**:
   - Set correct `BUZZER_TYPE` (ACTIVE/PASSIVE)
   - Set correct `LOW_LEVEL_TRIGGER` (0 or 1)
   - Re-upload sketch after changes

4. **Recovery mode**:
   - Hold BOOT button, press RESET, release RESET, release BOOT
   - Upload basic blink sketch to test
   - Use "Erase All Flash" in Arduino IDE

#### WiFi Connection Issues
**Symptoms**: "WiFi: DISCONNECTED", no IP address, can't access web interface
**Solutions**:
1. **Check credentials**:
   ```bash
   > WIFI MySSID MyPassword
   > SAVE
   > REBOOT
   ```

2. **Network issues**:
   - Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
   - Check router MAC filtering
   - Move closer to router
   - Try different WiFi channel (1, 6, or 11)

3. **Debug connection**:
   - Monitor Serial output during boot
   - Look for "[WiFi] Connected" message
   - Check assigned IP address

#### Time Not Syncing
**Symptoms**: "NO TIME SYNC" on display, wrong time
**Solutions**:
1. **Verify NTP**:
   ```
   > SYNC
   > NTP time.google.com
   > TZ +3
   ```

2. **Manual time set**:
   ```
   > TIME 2026-01-15 14:30:00
   ```

3. **Firewall issues**:
   - NTP uses UDP port 123
   - Check router/firewall allows NTP
   - Try different NTP server: `pool.ntp.org`, `time.windows.com`

#### Display Problems
**Symptoms**: Blank screen, garbled text, wrong content
**Solutions**:
1. **Brightness adjustment**:
   ```
   > BRIGHTNESS 255
   ```

2. **I2C issues**:
   - Check SCL/SCA connections (GPIO5/GPIO6)
   - Verify 3.3V power to OLED
   - Try different I2C address (usually 0x3C)

3. **Test display**:
   - Upload I2C scanner sketch
   - Check for I2C device at address 0x3C

#### Buzzer Not Working
**⚠️ CRITICAL: Voltage Warning** - ESP32 is 3.3V device!

**Active Buzzer Issues**:
1. **No sound**:
   - Check `#define BUZZER_TYPE BUZZER_ACTIVE`
   - Check polarity (positive to GPIO12)

2. **Constant sound**:
   - Check `#define LOW_LEVEL_TRIGGER` setting
   - Active buzzers usually need `LOW_LEVEL_TRIGGER 0`
   - Verify wiring not shorted

**Passive Buzzer Issues**:
1. **No melodies**:
   - Check `#define BUZZER_TYPE BUZZER_PASSIVE`
   - Set melody with `MELODY ALARM ...`
   - Test with `MELODY TEST C5 Q`
   - Verify PWM connection to GPIO12

2. **Low volume**:
   - Passive buzzers need amplification
   - Use transistor driver circuit
   - Connect VCC to 3.3V (use resistor divider for 5V)

3. **Wrong trigger type**:
   - Standard buzzers: `LOW_LEVEL_TRIGGER 0`
   - MH-FMD type: `LOW_LEVEL_TRIGGER 1`
   - Test both settings

#### Weather Display Issues
**Symptoms**: "No data", "Error", outdated weather
**Solutions**:
1. **City configuration**:
   ```
   > CITY Moscow
   > WEATHER
   ```

2. **API issues**:
   - wttr.in may be temporarily unavailable
   - Try different city name
   - Check internet connectivity

3. **Encoding problems**:
   - Use English city names when possible
   - Non-English names may need URL encoding
   - Weather updates every 10 minutes

#### Web Interface Problems
**Symptoms**: Can't connect, page won't load, features not working
**Solutions**:
1. **Find correct IP**:
   - Check Serial Monitor output
   - Press BOOT button to see IP on Info Screen 2
   - Use `STATUS` command

2. **Browser issues**:
   - Clear browser cache
   - Try different browser
   - Disable ad blockers for local IP

3. **Server not running**:
   - Check "[WebServer] Started" message
   - Ensure WiFi connected
   - Reboot device

#### NVS Storage Problems
**Symptoms**: Settings lost after reboot, alarms not saved
**Solutions**:
1. **Save properly**:
   ```
   > ALARM 07:30 Wake up S   # Note the 'S' flag
   > SAVE                    # Save all settings
   ```

2. **Corrupted NVS**:
   ```
   > ERASE                   # Complete wipe
   > REBOOT
   # Reconfigure everything
   ```

3. **Partition issues**:
   - Use correct partition scheme in Arduino IDE
   - "Default 4MB with spiffs" recommended
   - Avoid filling NVS (unlikely with this sketch)

---

### Advanced Configuration

#### Customizing Default Values
Edit these defaults in the sketch (lines ~100-130):

```cpp
// WiFi defaults
String defSSID = "YourDefaultSSID";
String defPASS = "YourDefaultPass";

// Time defaults
String defNTP = "pool.ntp.org";
long defGMTOffset = 3 * 3600;      // GMT+3
long defDaylightOffset = 0;

// Weather defaults
String defCity = "Moscow";

// Display defaults
const int defBrightness = 255;

// Melody defaults
String defAlarmMelody = "E5 E P S E5 E P S E5 Q P Q...";
String defTimerMelody = "G6 E A6 E G6 E F6 E P W";
```

#### Adding Custom Fonts
The sketch includes Cyrillic fonts. To add more:
1. Include font header: `#include <u8g2_font_unifont_t_cyrillic.h>`
2. Add to fontList array (line ~340)
3. Ensure proper UTF-8 encoding

#### Modifying Time Display
Change time format in `updateClockStrings()`:
```cpp
// Current format: "14:30"
snprintf(hhStr, sizeof(hhStr), "%02d", timeinfo.tm_hour);
snprintf(mmStr, sizeof(mmStr), "%02d", timeinfo.tm_min);

// For 12-hour format:
int hour12 = timeinfo.tm_hour % 12;
if (hour12 == 0) hour12 = 12;
snprintf(hhStr, sizeof(hhStr), "%2d", hour12);
```

#### Extending Alarm System
The alarm structure supports:
- Multiple alarms (currently single alarm)
- Snooze functionality
- Different melodies per alarm
- Volume control

To add multiple alarms:
1. Create array: `Alarm alarms[MAX_ALARMS];`
2. Modify NVS storage to handle array
3. Update web interface for multiple alarms

---

### API Reference (Web Interface)

All endpoints return UTF-8 encoded responses.

#### GET `/`
Returns main web interface HTML page.

#### GET `/status`
Returns comprehensive JSON status:
```json
{
  "time": "14:30",
  "date": "15.01.2026",
  "weekday": "Thu",
  "ssid": "HomeWiFi",
  "wifi": "Connected",
  "ip": "192.168.1.100",
  "timeSync": true,
  "ntpServer": "pool.ntp.org",
  "timezone": 3,
  "dstOffset": 0,
  "city": "Moscow",
  "brightness": 200,
  "buzzerType": "passive",
  "alarmMelody": "E5 E P S E5 E P S...",
  "timerMelody": "C5 E C5 E P H",
  "alarm": {
    "active": true,
    "hour": 7,
    "minute": 30,
    "text": "Wake up",
    "repeat": true,
    "saved": true,
    "type": "daily",
    "weekdays": 0,
    "date": ""
  },
  "timer": {
    "active": true,
    "remaining": 300,
    "text": "Tea"
  }
}
```

#### Alarm Endpoints
- `GET /alarm?time=HH:MM&type=daily&text=Message&repeat=1&save=1`
- `GET /alarm/clear`

#### Timer Endpoints
- `GET /timer?duration=SECONDS&text=Message`
- `GET /timer/clear`

#### System Endpoints
- `GET /sync?ntp=server&tz=3&dst=0`
- `GET /time?value=YYYY-MM-DD%20HH:MM:SS`
- `GET /weather?city=CityName`
- `GET /save?city=City&brightness=200`
- `GET /restore`
- `GET /erase`
- `GET /reboot`

#### Melody Endpoints (Passive Buzzer)
- `GET /melody/test?melody=C5%20Q%20D5%20Q`
- `GET /melody/save?alarm=melody&timer=melody`

---

### Technical Specifications

#### Power Consumption
- **Sleep mode**: 10mA (not implemented)
- **Idle (clock only)**: 80mA
- **WiFi active**: 120mA
- **Display on**: +20mA
- **Buzzer active**: +50mA
- **Total max**: ~200mA @ 3.3V

#### Memory Usage
- **Program flash**: ~500KB of 4MB
- **Runtime RAM**: ~100KB of 512KB
- **NVS storage**: ~2KB of 20KB
- **Web server**: Handles 4 simultaneous connections

#### Performance
- **Time update**: 1Hz (every second)
- **Display refresh**: 2-20Hz (depends on screen)
- **Button response**: 300ms debounce
- **WiFi reconnect**: 15 second timeout
- **NTP retry**: 30 attempts over 15 seconds
- **Weather update**: Every 10 minutes

#### Accuracy
- **Time keeping**: ±1 second/day (with NTP)
- **Timer accuracy**: ±0.01% (using esp_timer)
- **Alarm precision**: Exact to the second
- **NTP sync**: Within 50ms of server time

#### Environmental
- **Operating voltage**: 3.3V ±5%
- **Temperature range**: 0°C to 70°C
- **Humidity**: 20% to 80% non-condensing
- **WiFi range**: Up to 50m (open space)

---

### Safety Precautions

#### Electrical Safety
1. **3.3V ONLY** - ESP32-S3-Zero operates at 3.3V logic levels
2. **Voltage divider** - Required for 5V components
3. **Current limit** - GPIO pins limited to 40mA each
4. **ESD protection** - Handle boards carefully, use grounded workspace

#### Buzzer Safety
⚠️ **CRITICAL WARNINGS**:
- **Active buzzers**: Connect ONLY to 3.3V, not 5V
- **Passive buzzers**: May need current-limiting resistor
- **Volume**: High volume can damage hearing or buzzer
- **Duration**: Continuous operation may overheat buzzer

#### Thermal Considerations
- Maximum operating temperature: 70°C
- Avoid enclosed spaces without ventilation
- OLED displays generate minimal heat
- ESP32 may warm during WiFi operation

---

### Frequently Asked Questions

#### Q: Can I use a 5V buzzer?
**A**: Yes, but you MUST use a voltage divider or level shifter. Direct connection to 5V will damage the ESP32.

#### Q: How many alarms can I set?
**A**: Currently one alarm at a time. Multiple alarm support is planned for future versions.

#### Q: Does it work without WiFi?
**A**: Yes! The clock functions without WiFi, but won't sync time or show weather. Set time manually with `TIME` command.

#### Q: Can I change the language?
**A**: The interface is English, but displays support Cyrillic text. 

#### Q: How long do settings last without power?
**A**: NVS storage retains data for over 10 years without power.

#### Q: Can I power it from battery?
**A**: Yes, but consider power consumption (~200mA). A 2000mAh battery would last ~10 hours continuously.

#### Q: Is there a snooze function?
**A**: Not currently. Press BOOT to stop alarm, or use repeat flag for recurring alarms.

#### Q: Can I use a different display?
**A**: The code is written for SSD1306 128x64. Other displays would need code modifications.

---

### Updates and Version History

#### Version 1.0 (Current)
- Initial release with all core features
- Dual-zone OLED display
- Active/Passive buzzer support
- Web interface with real-time updates
- NTP time synchronization
- Weather display via wttr.in
- Serial console with command history
- NVS storage for settings
- Custom melody support (passive buzzer)


---
### License

This project is licensed under the **Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0)**.

#### You are free to:
- **Share** — copy and redistribute the material in any medium or format
- **Adapt** — remix, transform, and build upon the material

#### Under the following terms:
- **Attribution** — You must give appropriate credit to Anton Kovalev (CheshirCa), provide a link to the license, and indicate if changes were made.
- **NonCommercial** — You may not use the material for commercial purposes.

#### No additional restrictions:
- You may not apply legal terms or technological measures that legally restrict others from doing anything the license permits.

#### Full license text:
https://creativecommons.org/licenses/by-nc/4.0/legalcode

#### For commercial use:
Contact the author for licensing options.

---

### Acknowledgments

#### Libraries and Frameworks
- **ESP32 Arduino Core** by Espressif Systems
- **U8g2 Library** by olikraus - OLED display driver
- **Adafruit NeoPixel** by Adafruit - WS2812 control
- **Arduino Framework** - Foundation of the project

#### Services and APIs
- **wttr.in** by Igor Chubin - Weather data
- **NTP Pool Project** - Time synchronization
- **GitHub** - Version control and hosting

#### Hardware
- **Waveshare** for ESP32-S3-Zero board
- **Adafruit** for OLED display libraries
- **Community contributors** for testing and feedback

---

### Disclaimer

#### No Warranty
This software is provided "as is", without warranty of any kind, express or implied, including but not limited to the warranties of merchantability, fitness for a particular purpose and noninfringement. In no event shall the authors or copyright holders be liable for any claim, damages or other liability, whether in an action of contract, tort or otherwise, arising from, out of or in connection with the software or the use or other dealings in the software.

#### Safety Notice
- Always use proper electrical safety precautions
- This project involves mains electricity (via USB power supply)
- Ensure proper insulation and wiring
- Keep away from water and children
- Use appropriate fuses and circuit protection

#### Accuracy Notice
- Time accuracy depends on NTP server availability
- Weather data from third-party service (wttr.in)
- No guarantee of service availability or accuracy
- Use at your own risk for time-critical applications

---

<a name="russian"></a>
## Русский

### Продвинутая система будильника и таймера для ESP32-S3-Zero

Функциональный будильник на базе платы ESP32-S3-Zero с OLED-дисплеем, WiFi-подключением, синхронизацией времени NTP, отображением погоды, настраиваемыми звуковыми сигналами с поддержкой мелодий и двухзонным дисплеем.


---

### Возможности

####  Система будильника
- **Ежедневные будильники** - повтор каждый день в указанное время
- **Будильники по дням недели** - разные будильники для дней недели (Пн-Вс)
- **Будильники на конкретную дату** - одноразовые будильники
- **Произвольный текст** - персонализированные сообщения (до 30 символов, поддержка кириллицы)
- **Опция повтора** - будильник повторяется после срабатывания
- **Сохранение в NVS** - сохранение будильников в энергонезависимой памяти

####  Система таймера
- **Обратный отсчет** - установка таймеров от 1 секунды до 24 часов
- **Произвольный текст** - персонализированные метки таймера
- **Визуальная индикация** - светодиодный индикатор показывает статус
- **Микросекундная точность** - точное время с использованием esp_timer

####  Продвинутое управление зуммером
- **Поддержка активного зуммера** - простые зуммеры со встроенным генератором (HCM1203X) - только ВКЛ/ВЫКЛ
- **Поддержка пассивного зуммера** - пьезоэлементы с воспроизведением мелодий через ШИМ
- **Низко/высокоуровневый триггер** - настройка для разных типов зуммеров (MH-FMD использует низкий уровень)
- **Пользовательские мелодии** - создание и сохранение собственных мелодий для будильника и таймера (только пассивный зуммер)
- **Формат мелодий** - простая нотная нотация (например, "C5 Q D5 Q E5 H")
- **Тестирование мелодий** - проверка мелодий перед сохранением
- **Разные паттерны** - разные паттерны сигналов для будильника и таймера (активный зуммер)

####  Сетевые функции
- **WiFi-подключение** - подключение к беспроводной сети
- **Веб-интерфейс** - управление часами из любого браузера в вашей сети
- **Синхронизация NTP** - автоматическая синхронизация с интернет-серверами времени
- **Отображение погоды** - показывает текущую погоду с wttr.in
- **Настраиваемый город** - установка местоположения для точной погоды

####  Продвинутые функции дисплея
- **Двухцветный OLED** - 0.96" 128x64 SSD1306
  - Желтая зона (Y: 0-20) - строка состояния с датой и индикаторами
  - Синяя зона (Y: 21-64) - основная область с авто-масштабированием текста
- **Автоматический размер шрифта** - текст автоматически масштабируется под область дисплея
- **Перенос слов** - интеллектуальный перенос длинных сообщений
- **Несколько информационных экранов** - системная информация, сеть, погода
- **Регулируемая яркость** - настройка контрастности дисплея (0-255)
- **Поддержка кириллицы** - полная поддержка русских символов
- **Заставка** - пользовательский экран при запуске

####  Хранение данных
- **Энергонезависимая память (NVS)** - настройки сохраняются после перезагрузки
- **Сохраняемые настройки**:
  - Учетные данные WiFi
  - Часовой пояс и переход на летнее время
  - Конфигурация будильника
  - Яркость дисплея
  - Пользовательские мелодии (пассивный зуммер)
  - Город для погоды

####  Варианты управления
- **Веб-интерфейс** - полное управление из браузера с обновлениями в реальном времени
- **Последовательная консоль** - интерфейс командной строки через USB с историей команд
- **Физическая кнопка** - кнопка BOOT для навигации и остановки будильника
- **История команд** - клавиши-стрелки для навигации по предыдущим командам (консоль)
- **Авто-возврат к часам** - информационные экраны автоматически возвращаются после таймаута

####  Светодиодный индикатор
- **WS2812 RGB светодиод** - визуальный индикатор статуса
- **Цветовая кодировка**:
  - Синий: Будильник установлен
  - Зеленый: Таймер активен (мигает)
  - Красный: Будильник сработал (быстро мигает)
  - Желтый: Таймер сработал (быстро мигает)
  - Фиолетовый: Оба активны (чередуются)
- **Встроенный** - не требует внешней проводки

---

### Требования к оборудованию

#### Основная плата
- **ESP32-S3-Zero** (Waveshare)
  - Микроконтроллер ESP32-S3
  - Встроенный RGB-светодиод WS2812 (GPIO21)
  - Кнопка BOOT (GPIO0)
  - USB-C разъем для программирования и питания

#### Дисплей
- **OLED-дисплей SSD1306**
  - Размер: 0.96" (128x64 пикселей)
  - Интерфейс: I2C
  - Цвет: желтые/синие зоны
  - Подключение:
    - SCL → GPIO5
    - SDA → GPIO6
    - VCC → 3.3V
    - GND → GND

#### Зуммер (ВЫБЕРИТЕ ОДИН)
**⚠️ ВАЖНО: Подключайте к 3.3V, НЕ к 5V**

**Вариант 1: Активный зуммер (рекомендуется для начинающих)**
- Зуммер со встроенным генератором (HCM1203X)
- Только простая работа ВКЛ/ВЫКЛ
- НЕ может играть мелодии
- **Напряжение: ТОЛЬКО 3.3V** - Подключение к 5V может повредить ESP32
- Подключение:
  - Плюс → GPIO12
  - Минус → GND
  
**Вариант 2: Пассивный зуммер (для поддержки мелодий)**
- Пьезоэлемент без генератора
- Управление через ШИМ частоту
- Поддерживает пользовательские мелодии
- **Напряжение сигнала: 3.3V** - Используйте делитель напряжения для подключения к 5V
- Подключение:
  - Сигнал → GPIO12 (через модуль с транзистором или напрямую)
  - VCC → 3.3V (используйте резистивный делитель для 5V зуммеров)
  - GND → GND

**Зуммеры с низкоуровневым триггером (как MH-FMD):**
- Некоторые пассивные зуммеры активируются по низкому уровню сигнала
- Установите `#define LOW_LEVEL_TRIGGER 1` в коде
- Подключение такое же, как выше

#### Схема подключения
```
Распиновка ESP32-S3-Zero:
GPIO0  - Кнопка BOOT (встроенная)
GPIO5  - OLED SCL (тактовая линия I2C)
GPIO6  - OLED SDA (линия данных I2C)
GPIO12 - Выход зуммера (поддерживает ШИМ)
GPIO21 - WS2812 LED (встроенный)
3.3V   - Питание для OLED и зуммера
GND    - Общая земля
```

---

### Требования к программному обеспечению

#### Настройка Arduino IDE
1. **Установите Arduino IDE** (рекомендуется версия 2.0+)
2. **Добавьте поддержку платы ESP32**:
   - Файл → Настройки
   - Дополнительные ссылки для Менеджера плат: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
   - Инструменты → Плата → Менеджер плат
   - Найдите "esp32" и установите "esp32 by Espressif Systems"

3. **Выберите плату**:
   - Инструменты → Плата → ESP32 Arduino → Waveshare ESP32 S3 Zero

4. **Настройте параметры платы**:
   ```
   Плата: "Waveshare ESP32 S3 Zero"
   USB CDC On Boot: "Enabled"
   Частота CPU: "240MHz (WiFi)"
   Размер Flash: "4MB (32Mb)"
   Схема разделов: "Default 4MB with spiffs"
   PSRAM: "Disabled"
   Скорость загрузки: "921600"
   ```

#### Необходимые библиотеки
Установите эти библиотеки через Менеджер библиотек Arduino (Скетч → Подключить библиотеку → Управлять библиотеками):

1. **U8g2** (от oliver) - драйвер OLED-дисплея
   - Версия: 2.35.0 или новее
   - Поиск: "u8g2"
   - Необходима для: Управление дисплеем, кириллические шрифты

2. **Adafruit NeoPixel** (от Adafruit) - управление WS2812
   - Версия: 1.12.0 или новее
   - Поиск: "adafruit neopixel"
   - Необходима для: Управление RGB светодиодом

3. **Встроенные библиотеки ESP32** (установка не требуется):
   - WiFi
   - WebServer
   - HTTPClient
   - Preferences
   - функции time.h

---

### Установка и настройка

#### 1. Скачивание и открытие
```bash
git clone https://github.com/CheshirCa/ESP32-S3-Zero-alarm-clock.git
cd ESP32-S3-Zero-alarm-clock
```
Откройте `ESP32-C3-Z-Clock.ino` в Arduino IDE.

#### 2. Настройка типа зуммера (КРИТИЧЕСКИЙ ШАГ)
В скетче, примерно строки 40-55, настройте для ВАШЕГО оборудования:

**Для активного зуммера (простые сигналы):**
```cpp
#define BUZZER_TYPE BUZZER_ACTIVE  // Использовать активный зуммер
#define LOW_LEVEL_TRIGGER 0        // Стандартный HIGH триггер
```

**Для пассивного зуммера (мелодии):**
```cpp
#define BUZZER_TYPE BUZZER_PASSIVE  // Использовать пассивный зуммер
#define LOW_LEVEL_TRIGGER 0         // Стандартный HIGH триггер
```

**Для MH-FMD или зуммеров с низкоуровневым триггером:**
```cpp
#define BUZZER_TYPE BUZZER_PASSIVE  // Использовать пассивный зуммер
#define LOW_LEVEL_TRIGGER 1         // Низкоуровневый триггер
```

#### 3. Загрузка на плату
1. Подключите ESP32-S3-Zero через USB-C
2. Выберите правильный COM-порт (Инструменты → Порт)
3. Нажмите кнопку Загрузить (→)
4. Дождитесь сообщения "Done uploading"

#### 4. Первоначальная настройка через последовательную консоль
1. Откройте Serial Monitor (Инструменты → Serial Monitor, 115200 бод)
2. Дождитесь сообщения загрузки: "=== ESP32-S3-Zero Alarm Clock ==="
3. Настройте базовые параметры:
   ```
   > WIFI ВашSSID ВашПароль
   > CITY Москва
   > TZ +3
   > SAVE
   ```
4. Устройство подключится к WiFi и синхронизирует время автоматически

---

### Руководство пользователя

#### Последовательность первой загрузки
1. Показывается заставка с информацией об авторе
2. Появляется тестовый паттерн OLED
3. Загружается конфигурация из NVS (или значения по умолчанию)
4. Попытка подключения к WiFi (если настроено)
5. Синхронизация времени NTP
6. Обновление погоды (если WiFi подключен)
7. Появляется главный экран часов

#### Веб-интерфейс
1. **Найдите IP-адрес**:
   - Проверьте Serial Monitor: "[WebServer] Access at: http://192.168.x.x"
   - Или нажмите кнопку BOOT для перехода к Инфо экрану 2

2. **Доступ к интерфейсу**:
   - Откройте браузер на любом устройстве в той же WiFi сети
   - Перейдите: `http://<ip_устройства>`
   - Пароль не требуется

3. **Функции веб-интерфейса**:
   - **Часы реального времени** - Большое отображение времени с авто-обновлением
   - **Системные настройки** - WiFi, NTP, часовой пояс, яркость
   - **Управление будильниками** - Установка ежедневных/недельных/датированных будильников
   - **Управление таймерами** - Запуск таймеров обратного отсчета
   - **Настройки погоды** - Настройка города и обновление погоды
   - **Редактор мелодий** - Создание и тестирование пользовательских мелодий (только пассивный зуммер)
   - **Системные действия** - Сохранить, восстановить, стереть, перезагрузить

4. **Советы по редактору мелодий**:
   - Вводите мелодии в формате: "C5 Q D5 Q E5 H"
   - Нажмите "Test" чтобы услышать перед сохранением
   - Используйте "Save Melodies" для сохранения в NVS
   - Мелодии сохраняются после перезагрузки

#### Команды последовательной консоли

##### Общие команды
- `HELP` - Показать все доступные команды
- `STATUS` - Отобразить детальный статус системы
- `REBOOT` - Перезагрузить устройство

##### Управление временем
- `TIME 2026-01-15 14:30:00` - Установить время вручную (ГГГГ-ММ-ДД ЧЧ:ММ:СС)
- `SYNC` - Принудительная синхронизация времени NTP
- `TZ +3` - Установить смещение часового пояса (GMT+3 для Москвы)
- `DST +0` - Установить смещение летнего времени (0 для России)
- `NTP pool.ntp.org` - Изменить NTP-сервер

##### Сетевая конфигурация
- `WIFI МойSSID МойПароль` - Установить учетные данные WiFi
- `CITY Москва` - Установить город для погоды (поддерживает международные названия)

##### Управление дисплеем
- `BRIGHTNESS 200` - Установить контрастность OLED (0-255, 255=макс)
- `DISPLAY TEST` - Показать тестовый паттерн

##### Команды будильника
```
ALARM [ГГГГ-ММ-ДД|1234567] ЧЧ:ММ [ТЕКСТ] [R] [S]
```
- **Форматы даты**:
  - `07:30` - Ежедневно в 7:30 утра
  - `2026-12-31 23:59` - Конкретная дата
  - `12345 08:00` - Рабочие дни Пн-Пт в 8:00
- **Флаги**:
  - `R` - Повторять после срабатывания
  - `S` - Сохранить в NVS (сохраняется после перезагрузки)
- **Примеры**:
  - `ALARM 07:30 Подъем! R S` - Ежедневный будильник, повтор, сохранение
  - `ALARM 2026-12-31 23:59 С Новым Годом! S` - Новогодний будильник
  - `ALARM 67 09:00 Выходной S` - Суббота и воскресенье в 9:00
  - `ALARM CLEAR` - Удалить текущий будильник

##### Команды таймера
```
TIMER [ЧЧ:]ММ:СС [ТЕКСТ]
TIMER СС [ТЕКСТ]
```
- **Форматы**:
  - `300` - 300 секунд (5 минут)
  - `05:00` - 5 минут
  - `01:30:00` - 1 час 30 минут
- **Примеры**:
  - `TIMER 300 Чай готов` - 5-минутный таймер
  - `TIMER 45:00 Пицца` - 45-минутный таймер
  - `TIMER CLEAR` - Остановить текущий таймер

##### Управление хранилищем
- `SAVE` - Сохранить ВСЕ текущие настройки в NVS
- `RESTORE` - Загрузить настройки из NVS
- `ERASE` - Стереть ВСЕ данные NVS (сброс к заводским)

##### Команды зуммера и мелодий (только пассивный зуммер)
- `MELODY ALARM C5 Q D5 Q E5 H` - Установить мелодию будильника
- `MELODY TIMER C5 E C5 E P H` - Установить мелодию таймера
- `MELODY TEST C5 Q D5 Q E5 H` - Протестировать мелодию
- `MELODY SAVE` - Сохранить мелодии в NVS

**Формат мелодии**:
- **Ноты**: `C`, `C#`, `D`, `D#`, `E`, `F`, `F#`, `G`, `G#`, `A`, `A#`, `B`, `P` (пауза)
- **Октавы**: `3`, `4`, `5`, `6`, `7` (средняя до = C4)
- **Длительности**: `W` (целая=1000мс), `H` (половинная=500мс), `Q` (четвертная=250мс), `E` (восьмая=125мс), `S` (шестнадцатая=63мс)
- **Примеры мелодий**:
  - Простой сигнал: `C5 Q P Q C5 Q`
  - Оповещение: `E5 E E5 E E5 Q G5 Q C5 Q D5 Q E5 H`
  - Сигнал таймера: `C5 E C5 E P H`

#### Работа с кнопкой
- **Однократное нажатие**: Переключение экранов (Часы → Инфо1 → Инфо2 → Инфо3 → Часы)
- **При срабатывании будильника/таймера**: Нажмите чтобы остановить звук и вернуться к часам
- **Авто-возврат**: Информационные экраны возвращаются к часам через 10 секунд

#### Цвета светодиодного индикатора
- **Постоянный синий** - Будильник установлен
- **Мигающий зеленый** - Таймер активен (интервал 500мс)
- **Быстро мигающий красный** - Будильник сработал (интервал 250мс)
- **Быстро мигающий желтый** - Таймер сработал (интервал 250мс)
- **Чередующийся синий/зеленый** - Оба будильник и таймер активны
- **Выключен** - Нет будильников или таймеров

---

### Экраны дисплея

#### Главный экран часов
- Большое цифровое время (шрифт 38px) с мигающим двоеточием
- Дата в верхней желтой зоне (формат ДД.ММ.ГГГГ)
- Индикаторы: `A` (будильник активен), `T` (таймер активен)
- Статус синхронизации времени (двоеточие мигает только при синхронизации)

#### Информационный экран 1 - Статус системы
- Текущий день недели
- Статус будильника со временем, если активен
- Статус таймера с оставшимся временем
- Статус подключения WiFi
- Авто-возврат к часам через 10 секунд

#### Информационный экран 2 - Сетевая информация
- SSID WiFi (обрезается если длинный)
- IP-адрес
- Статус синхронизации времени
- Свободная RAM в килобайтах

#### Информационный экран 3 - Отображение погоды
- Название города
- Текущая температура (°C)
- Погодные условия
- Авто-обновление каждые 10 минут
- Показывает "Нет данных" если WiFi не подключен

#### Экран срабатывания будильника/таймера
- Полноэкранное пользовательское сообщение
- Инструкция "Press BOOT!" в желтой зоне
- Сообщение автоматически масштабируется под синюю зону
- Используются разные шрифты от самого большого к самому маленькому
- Поддерживает кириллический текст

---

### Примеры конфигурации

#### Пример 1: Полная утренняя рутина
```
Настройка через последовательную консоль:
> WIFI ДомашнийWiFi МойПароль
> CITY Москва
> TZ +3
> NTP ru.pool.ntp.org
> ALARM 12345 07:00 Подъем! R S
> ALARM 67 08:30 Выходной R S
> BRIGHTNESS 150
> CITY Москва
> SAVE

Веб-интерфейс:
- Установите яркость дисплея на 150
- Проверьте часовой пояс GMT+3
- Протестируйте мелодию будильника
- Установите частоту обновления погоды
```

#### Пример 2: Помощник для готовки
```
> TIMER 15:00 Проверить пасту
[Через 15 минут - срабатывает таймер]
Дисплей показывает: "ПРОВЕРИТЬ ПАСТУ" крупным шрифтом
Зуммер играет мелодию таймера
Нажмите кнопку BOOT чтобы остановить

Веб-интерфейс:
- Создавайте несколько таймеров последовательно
- Используйте описательный текст: "Чай", "Пицца", "Торт"
- Сохраняйте часто используемые таймеры как пресеты
```

#### Пример 3: Создание пользовательской мелодии
```
Для пассивного зуммера:
> MELODY ALARM E5 E P S E5 E P S E5 Q P Q E5 E P S E5 E P S E5 Q P Q
> MELODY TEST E5 E P S E5 E P S E5 Q P Q
> MELODY SAVE

Редактор мелодий в веб-интерфейсе:
- Введите: C5 Q D5 Q E5 Q F5 Q G5 Q A5 Q B5 Q C6 H
- Нажмите "Test Alarm Melody" для предварительного прослушивания
- Нажмите "Save Melodies" для сохранения
- Мелодия сохраняется в NVS, сохраняется после перезагрузки
```

#### Пример 4: Особые случаи
```
Напоминание о дне рождения:
> ALARM 2026-07-15 09:00 С Днем Рождения! S

Будильник для встречи:
> ALARM 35 14:30 Совещание команды S  (Вт/Чт в 14:30)

Таймер для медитации:
> TIMER 20:00 Медитация
> ALARM 1234567 21:00 Вечерняя медитация R S
```

---

### Руководство по созданию мелодий

#### Понимание формата
Мелодии используют простой формат с пробелами: `НотаОктава Длительность`
- `E5` - Нота E, 5-я октава (659 Гц)
- `Q` - Четвертная нота (250мс)
- `P` - Пауза (тишина)
- `S` - Шестнадцатая нота (63мс)

#### Частоты нот
| Нота | Октава 4 | Октава 5 | Октава 6 |
|------|----------|----------|----------|
| C    | 262 Гц   | 523 Гц   | 1047 Гц  |
| D    | 294 Гц   | 587 Гц   | 1175 Гц  |
| E    | 330 Гц   | 659 Гц   | 1319 Гц  |
| F    | 349 Гц   | 698 Гц   | 1397 Гц  |
| G    | 392 Гц   | 784 Гц   | 1568 Гц  |
| A    | 440 Гц   | 880 Гц   | 1760 Гц  |
| B    | 494 Гц   | 988 Гц   | 1976 Гц  |

#### Популярные мелодии

**Простое оповещение** (хорошо для будильников):
```
E5 E P S E5 E P S E5 Q P Q
```
Перевод: Два коротких сигнала, пауза, один более длинный сигнал

**Сигнал таймера**:
```
C5 E C5 E P H
```
Перевод: Два быстрых сигнала, длинная пауза, повтор

**Восходящая гамма** (привлекает внимание):
```
C5 Q D5 Q E5 Q F5 Q G5 Q A5 Q B5 Q C6 H
```

**Пользовательская мелодия**:
```
E5 Q G5 Q E5 Q C5 Q D5 Q E5 H P H
```

#### Советы для лучших мелодий
1. **Начните с простого** - Используйте 3-5 нот для базовых оповещений
2. **Добавьте паузы** - Паузы создают ритм: `C5 Q P Q C5 Q`
3. **Смешивайте длительности** - Комбинируйте длинные и короткие ноты
4. **Тестируйте часто** - Используйте `MELODY TEST` перед сохранением
5. **Учитывайте громкость** - Более высокие октавы (6,7) громче
6. **Делайте короткими** - Максимум 10-20 нот для ясности

---

### Устранение неполадок

#### Устройство не загружается или крашится
**Симптомы**: Постоянная перезагрузка, "Guru Meditation Error", пустой дисплей
**Решения**:
1. **Проверьте питание**:
   - Используйте качественный USB кабель (данные + питание)
   - Избегайте USB-хабов, подключайте напрямую к компьютеру
   - Обеспечьте минимум 5V 1A блок питания

2. **Проверьте проводку**:
   - OLED SCL → GPIO5, SDA → GPIO6
   - Зуммер → GPIO12 (проверьте полярность)
   - Все земли соединены вместе

3. **Конфигурация кода**:
   - Установите правильный `BUZZER_TYPE` (ACTIVE/PASSIVE)
   - Установите правильный `LOW_LEVEL_TRIGGER` (0 или 1)
   - Перезагрузите скетч после изменений

4. **Режим восстановления**:
   - Удерживайте кнопку BOOT, нажмите RESET, отпустите RESET, отпустите BOOT
   - Загрузите простой скетч blink для тестирования
   - Используйте "Erase All Flash" в Arduino IDE

#### Проблемы с подключением WiFi
**Симптомы**: "WiFi: DISCONNECTED", нет IP-адреса, нет доступа к веб-интерфейсу
**Решения**:
1. **Проверьте учетные данные**:
   ```bash
   > WIFI МойSSID МойПароль
   > SAVE
   > REBOOT
   ```

2. **Проблемы с сетью**:
   - Убедитесь, что сеть 2.4ГГц (ESP32 не поддерживает 5ГГц)
   - Проверьте фильтрацию MAC на роутере
   - Подвиньтесь ближе к роутеру
   - Попробуйте разные каналы WiFi (1, 6, или 11)

3. **Отладка подключения**:
   - Мониторьте вывод Serial во время загрузки
   - Ищите сообщение "[WiFi] Connected"
   - Проверьте назначенный IP-адрес

#### Время не синхронизируется
**Симптомы**: "NO TIME SYNC" на дисплее, неправильное время
**Решения**:
1. **Проверьте NTP**:
   ```
   > SYNC
   > NTP time.google.com
   > TZ +3
   ```

2. **Ручная установка времени**:
   ```
   > TIME 2026-01-15 14:30:00
   ```

3. **Проблемы с фаерволом**:
   - NTP использует UDP порт 123
   - Проверьте, что роутер/фаервол разрешает NTP
   - Попробуйте разные NTP-серверы: `pool.ntp.org`, `time.windows.com`

#### Проблемы с дисплеем
**Симптомы**: Пустой экран, искаженный текст, неправильное содержание
**Решения**:
1. **Регулировка яркости**:
   ```
   > BRIGHTNESS 255
   ```

2. **Проблемы с I2C**:
   - Проверьте подключения SCL/SCA (GPIO5/GPIO6)
   - Убедитесь в питании 3.3V для OLED
   - Попробуйте другой I2C адрес (обычно 0x3C)

3. **Тест дисплея**:
   - Загрузите скетч I2C сканера
   - Проверьте наличие I2C устройства по адресу 0x3C

#### Зуммер не работает
**⚠️ КРИТИЧЕСКОЕ ПРЕДУПРЕЖДЕНИЕ**: ESP32 - устройство на 3.3V!

**Проблемы с активным зуммером**:
1. **Нет звука**:
   - Проверьте `#define BUZZER_TYPE BUZZER_ACTIVE`
   - Убедитесь, что зуммер подключен к 3.3V (НЕ к 5V!)
   - Проверьте полярность (плюс к GPIO12)

2. **Постоянный звук**:
   - Проверьте настройку `#define LOW_LEVEL_TRIGGER`
   - Активные зуммеры обычно требуют `LOW_LEVEL_TRIGGER 0`
   - Убедитесь, что проводка не замкнута

**Проблемы с пассивным зуммером**:
1. **Нет мелодий**:
   - Проверьте `#define BUZZER_TYPE BUZZER_PASSIVE`
   - Установите мелодию командой `MELODY ALARM ...`
   - Протестируйте командой `MELODY TEST C5 Q`
   - Проверьте ШИМ подключение к GPIO12

2. **Низкая громкость**:
   - Пассивные зуммеры требуют усиления
   - Используйте схему с транзисторным драйвером
   - Подключите VCC к 3.3V (используйте резистивный делитель для 5V)

3. **Неправильный тип триггера**:
   - Стандартные зуммеры: `LOW_LEVEL_TRIGGER 0`
   - Тип MH-FMD: `LOW_LEVEL_TRIGGER 1`
   - Протестируйте обе настройки

#### Проблемы с отображением погоды
**Симптомы**: "Нет данных", "Ошибка", устаревшая погода
**Решения**:
1. **Конфигурация города**:
   ```
   > CITY Москва
   > WEATHER
   ```

2. **Проблемы с API**:
   - wttr.in может быть временно недоступен
   - Попробуйте другое название города
   - Проверьте подключение к интернету

3. **Проблемы с кодировкой**:
   - Используйте английские названия городов когда возможно
   - Неанглийские названия могут требовать URL кодирования
   - Погода обновляется каждые 10 минут

#### Проблемы с веб-интерфейсом
**Симптомы**: Не могу подключиться, страница не загружается, функции не работают
**Решения**:
1. **Найдите правильный IP**:
   - Проверьте вывод Serial Monitor
   - Нажмите кнопку BOOT чтобы увидеть IP на Инфо экране 2
   - Используйте команду `STATUS`

2. **Проблемы с браузером**:
   - Очистите кеш браузера
   - Попробуйте другой браузер
   - Отключите блокировщики рекламы для локального IP

3. **Сервер не запущен**:
   - Проверьте сообщение "[WebServer] Started"
   - Убедитесь, что WiFi подключен
   - Перезагрузите устройство

#### Проблемы с хранилищем NVS
**Симптомы**: Настройки потеряны после перезагрузки, будильники не сохраняются
**Решения**:
1. **Сохраняйте правильно**:
   ```
   > ALARM 07:30 Подъем S   # Обратите внимание на флаг 'S'
   > SAVE                    # Сохранить все настройки
   ```

2. **Поврежденный NVS**:
   ```
   > ERASE                   # Полная очистка
   > REBOOT
   # Перенастройте все заново
   ```

3. **Проблемы с разделами**:
   - Используйте правильную схему разделов в Arduino IDE
   - Рекомендуется "Default 4MB with spiffs"
   - Избегайте заполнения NVS (маловероятно с этим скетчем)

---

### Часто задаваемые вопросы

#### В: Можно ли использовать 5V зуммер?
**О**: Да, но вы ДОЛЖНЫ использовать делитель напряжения или сдвиг уровней. Прямое подключение к 5V повредит ESP32.

#### В: Сколько будильников можно установить?
**О**: В настоящее время один будильник одновременно. Поддержка нескольких будильников планируется в будущих версиях.

#### В: Работает ли без WiFi?
**О**: Да! Часы функционируют без WiFi, но не будут синхронизировать время или показывать погоду. Установите время вручную командой `TIME`.

#### В: Можно ли изменить язык?
**О**: Интерфейс на английском, но дисплей поддерживает кириллический текст. Дни недели показываются русскими сокращениями.

#### В: Как долго хранятся настройки без питания?
**О**: Хранилище NVS сохраняет данные более 10 лет без питания.

#### В: Можно ли питать от батареи?
**О**: Да, но учитывайте потребление энергии (~200мА). Батарея 2000мАч проработает ~10 часов непрерывно.

#### В: Есть ли функция повтора (snooze)?
**О**: В настоящее время нет. Нажмите BOOT чтобы остановить будильник, или используйте флаг повтора для повторяющихся будильников.

#### В: Можно ли использовать другой дисплей?
**О**: Код написан для SSD1306 128x64. Для других дисплеев потребуется модификация кода.

---

### Обновления и история версий

#### Версия 1.0 (Текущая)
- Первый релиз со всеми основными функциями
- Двухзонный OLED дисплей
- Поддержка активного/пассивного зуммера
- Веб-интерфейс с обновлениями в реальном времени
- Синхронизация времени NTP
- Отображение погоды через wttr.in
- Последовательная консоль с историей команд
- Хранилище NVS для настроек
- Поддержка пользовательских мелодий (пассивный зуммер)


---

### Лицензия

Этот проект лицензирован по **Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0)**.

#### Вы можете:
- **Делиться** — копировать и распространять материал на любом носителе или в любом формате
- **Адаптировать** — редактировать, преобразовывать и создавать на основе материала

#### На следующих условиях:
- **С указанием авторства** — Вы должны указать авторство Антона Ковалева (CheshirCa), предоставить ссылку на лицензию и указать, были ли внесены изменения.
- **Некоммерческая** — Вы не можете использовать материал в коммерческих целях.

#### Без дополнительных ограничений:
- Вы не можете применять правовые условия или технологические меры, которые юридически ограничивают других делать что-либо, разрешенное лицензией.

#### Полный текст лицензии:
https://creativecommons.org/licenses/by-nc/4.0/legalcode

#### Для коммерческого использования:
Свяжитесь с автором для получения вариантов лицензирования.

---

### Благодарности

#### Библиотеки и фреймворки
- **ESP32 Arduino Core** от Espressif Systems
- **Библиотека U8g2** от olikraus - драйвер OLED дисплея
- **Adafruit NeoPixel** от Adafruit - управление WS2812
- **Arduino Framework** - основа проекта

#### Сервисы и API
- **wttr.in** от Igor Chubin - данные о погоде
- **NTP Pool Project** - синхронизация времени
- **GitHub** - контроль версий и хостинг

#### Оборудование
- **Waveshare** за плату ESP32-S3-Zero
- **Adafruit** за библиотеки OLED дисплеев
- **Сообщество контрибьюторов** за тестирование и обратную связь


---

### Отказ от ответственности

#### Без гарантий
Это программное обеспечение предоставляется "как есть", без каких-либо гарантий, выраженных или подразумеваемых, включая, но не ограничиваясь гарантиями товарной пригодности, пригодности для определенной цели и ненарушения прав. Ни при каких обстоятельствах авторы или держатели авторских прав не несут ответственности за какие-либо претензии, убытки или иную ответственность, будь то в результате действия контракта, деликта или иного, возникающие из, в связи с или в связи с программным обеспечением или использованием или другими операциями с программным обеспечением.

#### Уведомление о безопасности
- Всегда соблюдайте надлежащие меры электробезопасности
- Этот проект включает сетевую электроэнергию (через USB блок питания)
- Обеспечьте надлежащую изоляцию и проводку
- Держите вдали от воды и детей
- Используйте соответствующие предохранители и защиту цепи

#### Уведомление о точности
- Точность времени зависит от доступности NTP сервера
- Данные о погоде от стороннего сервиса (wttr.in)
- Нет гарантии доступности или точности сервиса
- Используйте на свой риск для критичных по времени приложений

---

### Об авторе

**Антон Ковалев (CheshirCa)**
- Инженер встраиваемых систем
- Энтузиаст open source
- Создатель различных проектов на ESP32
- Фокус на практичных, полезных IoT устройствах

**Контакт**: Через GitHub Issues или Discussions

**Telegram**: @CheshirCa

